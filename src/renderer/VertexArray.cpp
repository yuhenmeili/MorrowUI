//
// Created by lance on 2023/12/4.
//

#include "VertexArray.h"

#include "BatchDataDefine.h"
#include "GlobalObject.h"
#include "RenderDeviceProxy.h"
#include "base/Mesh.h"
#include "base/MeshFilter.h"

namespace morrow {
VertexArray::~VertexArray() {
    if (m_vbo) {
        RENDERINGTHREAD->deleteVBO(m_vbo);
        m_vbo = nullptr;
    }
}

bool VertexArray::needsMeshUpload(
    GPUProgram* program,
    const std::vector<std::shared_ptr<MeshFilter>>& meshFilters) const {
    if (!m_vbo || m_uploadedProgram != program) return true;

    size_t validMeshCount = 0;
    for (const auto& meshFilter : meshFilters) {
        if (meshFilter && meshFilter->getMesh()) {
            ++validMeshCount;
        }
    }

    if (validMeshCount != m_uploadedMeshes.size()) return true;

    size_t uploadedIndex = 0;
    for (const auto& meshFilter : meshFilters) {
        if (!meshFilter) continue;
        const auto& mesh = meshFilter->getMesh();
        if (!mesh) continue;

        const auto uploadedMesh = m_uploadedMeshes[uploadedIndex].mesh.lock();
        if (uploadedMesh.get() != mesh.get() ||
            m_uploadedMeshes[uploadedIndex].revision != mesh->getRevision()) {
            return true;
        }
        ++uploadedIndex;
    }
    return false;
}

void VertexArray::recordUploadedMeshes(GPUProgram* program, const std::vector<std::shared_ptr<MeshFilter>>& meshFilters) {
    m_uploadedProgram = program;
    m_uploadedMeshes.clear();
    m_uploadedMeshes.reserve(meshFilters.size());
    for (const auto& meshFilter : meshFilters) {
        if (!meshFilter) continue;
        const auto& mesh = meshFilter->getMesh();
        if (!mesh) continue;
        m_uploadedMeshes.push_back({mesh, mesh->getRevision()});
    }
}

void VertexArray::updateFromMeshes(std::shared_ptr<FrameState> frameState, GPUProgram* program, const std::vector<std::shared_ptr<MeshFilter>>& meshFilters) {
    (void)frameState;
    if (!needsMeshUpload(program, meshFilters)) {
        return;
    }

    if (!m_vbo) {
        m_vbo = RENDERINGTHREAD->createVBO();
    }
    // 计算总的顶点数和索引数
    size_t totalVertexCount = 0;
    size_t totalIndexCount = 0;
    // 确定哪些属性存在（检查所有mesh，确保一致性）
    bool hasColors = false;
    bool hasUVs = false;
    bool hasNormals = false;
    PrimitiveType drawMode = PrimitiveType::TRIANGLES;

    for (const auto& meshFilter : meshFilters) {
        if (meshFilter && meshFilter->getMesh()) {
            const std::shared_ptr<Mesh>& mesh = meshFilter->getMesh();

            totalVertexCount += mesh->getVertexCount();
            totalIndexCount += mesh->getIndexCount();

            hasColors = hasColors || !mesh->getColors().empty();
            hasUVs = hasUVs || !mesh->getUVs().empty();
            hasNormals = hasNormals || !mesh->getNormals().empty();
            drawMode = mesh->getDrawMode(); // 使用最后一个有效的mesh的绘制模式
        }
    }

    // 从回收池获取或新建 VBOData（GPU Fence 完成后回池）
    auto* pool = RENDERINGTHREAD->getVBODataRecyclePool();
    VBODataSharedPtr vboData = pool ? pool->acquire() : std::make_shared<VBOData>();
    // 如果没有顶点数据，直接返回
    if (totalVertexCount == 0) {
        RENDERINGTHREAD->updateVBO(program, m_vbo, vboData);
        recordUploadedMeshes(program, meshFilters);
        return;
    }

    // 清空旧数据
    vboData->attributes.clear();
    vboData->vertexData.clear();
    vboData->indices.clear();

    vboData->vertexCount = totalVertexCount;
    vboData->indexCount = totalIndexCount;
    vboData->drawMode = drawMode;

    // 计算各属性的数据大小
    size_t posSize = totalVertexCount * sizeof(Vector3);
    size_t colorSize = hasColors ? totalVertexCount * sizeof(Vector4) : 0;
    size_t uvSize = hasUVs ? totalVertexCount * sizeof(Vector2) : 0;
    size_t normalSize = hasNormals ? totalVertexCount * sizeof(Vector3) : 0;
    size_t batchIdSize = totalVertexCount * sizeof(float); // 每个顶点一个batch ID
    size_t totalSize = posSize + colorSize + uvSize + normalSize + batchIdSize;

    // 预分配内存
    vboData->vertexData.resize(totalSize);
    vboData->indices.resize(totalIndexCount);

    // 定义属性布局
    size_t offset = 0;
    // 位置属性（必须存在）
    vboData->attributes.push_back({VertexAttributeType::Position, offset, sizeof(Vector3)});
    offset += posSize;
    // Batch ID属性
    vboData->attributes.push_back({VertexAttributeType::BatchID, offset, sizeof(float)});
    offset += batchIdSize;
    // 颜色属性（可选）
    if (hasColors) {
        vboData->attributes.push_back({VertexAttributeType::Color, offset, sizeof(Vector4)});
        offset += colorSize;
    }
    // UV属性（可选）
    if (hasUVs) {
        vboData->attributes.push_back({VertexAttributeType::UV, offset, sizeof(Vector2)});
        offset += uvSize;
    }
    // 法线属性（可选）
    if (hasNormals) {
        vboData->attributes.push_back({VertexAttributeType::Normal, offset, sizeof(Vector3)});
    }

    // 分别复制每个mesh的数据
    size_t vertexOffset = 0;
    size_t indexOffset = 0;
    uint32_t batchId = 0;

    for (const auto& meshFilter : meshFilters) {
        if (!meshFilter || !meshFilter->getMesh()) continue;

        const std::shared_ptr<Mesh>& mesh = meshFilter->getMesh();
        size_t meshVertexCount = mesh->getVertexCount();
        size_t meshIndexCount = mesh->getIndexCount();

        if (meshVertexCount == 0) continue;

        // 计算各个属性在vertexData中的偏移量
        size_t currentPosOffset = vertexOffset * sizeof(Vector3);
        size_t currentBatchOffset = posSize + vertexOffset * sizeof(float);
        size_t currentColorOffset = posSize + batchIdSize + vertexOffset * sizeof(Vector4);
        size_t currentUVOffset = posSize + batchIdSize + colorSize + vertexOffset * sizeof(Vector2);
        size_t currentNormalOffset = posSize + batchIdSize + colorSize + uvSize + vertexOffset * sizeof(Vector3);

        // 复制顶点位置数据
        if (!mesh->getVertices().empty()) {
            memcpy(vboData->vertexData.data() + currentPosOffset,
                   mesh->getVertices().data(),
                   meshVertexCount * sizeof(Vector3));
        }

        // 复制Batch ID数据 - 为当前mesh的所有顶点设置相同的batch ID
        auto batchIdFloat = static_cast<float>(batchId);
        for (size_t i = 0; i < meshVertexCount; ++i) {
            memcpy(vboData->vertexData.data() + currentBatchOffset + i * sizeof(float),
                   &batchIdFloat,
                   sizeof(float));
        }

        // 复制颜色数据
        if (hasColors && !mesh->getColors().empty()) {
            memcpy(vboData->vertexData.data() + currentColorOffset,
                   mesh->getColors().data(),
                   meshVertexCount * sizeof(Vector4));
        } else if (hasColors) {
            // 如果该mesh没有颜色数据但其他mesh有，填充默认颜色
            Vector4 defaultColor(1.0f, 1.0f, 1.0f, 1.0f);
            for (size_t i = 0; i < meshVertexCount; ++i) {
                memcpy(vboData->vertexData.data() + currentColorOffset + i * sizeof(Vector4),
                       &defaultColor,
                       sizeof(Vector4));
            }
        }

        // 复制UV数据
        if (hasUVs && !mesh->getUVs().empty()) {
            memcpy(vboData->vertexData.data() + currentUVOffset,
                   mesh->getUVs().data(),
                   meshVertexCount * sizeof(Vector2));
        } else if (hasUVs) {
            // 如果该mesh没有UV数据但其他mesh有，填充默认UV
            Vector2 defaultUV(0.0f, 0.0f);
            for (size_t i = 0; i < meshVertexCount; ++i) {
                memcpy(vboData->vertexData.data() + currentUVOffset + i * sizeof(Vector2),
                       &defaultUV,
                       sizeof(Vector2));
            }
        }

        // 复制法线数据
        if (hasNormals && !mesh->getNormals().empty()) {
            memcpy(vboData->vertexData.data() + currentNormalOffset,
                   mesh->getNormals().data(),
                   meshVertexCount * sizeof(Vector3));
        } else if (hasNormals) {
            // 如果该mesh没有法线数据但其他mesh有，填充默认法线
            Vector3 defaultNormal(0.0f, 1.0f, 0.0f);
            for (size_t i = 0; i < meshVertexCount; ++i) {
                memcpy(vboData->vertexData.data() + currentNormalOffset + i * sizeof(Vector3),
                       &defaultNormal,
                       sizeof(Vector3));
            }
        }

        // 复制索引数据并调整索引值
        if (meshIndexCount > 0 && !mesh->getIndices().empty()) {
            const std::vector<int16_t>& meshIndices = mesh->getIndices();
            for (size_t i = 0; i < meshIndexCount; ++i) {
                vboData->indices[indexOffset + i] = static_cast<uint32_t>(meshIndices[i]) + static_cast<uint32_t>(vertexOffset);
            }
            indexOffset += meshIndexCount;
        }

        vertexOffset += meshVertexCount;
        batchId++;
    }

    // 更新VBO
    RENDERINGTHREAD->updateVBO(program, m_vbo, vboData);
    recordUploadedMeshes(program, meshFilters);
}


void VertexArray::draw(std::shared_ptr<FrameState> frameState, int32_t instanceCount) {
    frameState->drawCallCount++;
    RENDERINGTHREAD->drawVBO(m_vbo, instanceCount);
}
}