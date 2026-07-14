//
// Created by 0060328 on 25-10-20.
//

#include "BatchManager.h"

#include <algorithm>

#include "BatchDataDefine.h"
#include "Material.h"
#include "OrthographicCamera.h"
#include "RenderBatchPool.h"
#include "SSBOManager.h"
#include "UniformBuffer.h"
#include "VertexArray.h"
#include "base/Component.inl"
#include "base/MeshFilter.h"
#include "base/MeshRenderer.h"
#include "base/Transform.h"
#include "base/Widget.h"

namespace morrow {

BatchManager::BatchManager() {
    m_ubo = std::make_shared<UniformBuffer>();
}

// ---------------------------------------------------------------------------
// 收集阶段：仅记录，不做合批
// ---------------------------------------------------------------------------
void BatchManager::addRenderable(std::shared_ptr<Material> material,
                                  std::shared_ptr<MeshFilter> meshFilter,
                                  std::shared_ptr<Transform> transform) {
    RenderableItem item;
    item.material = std::move(material);
    item.meshFilter = std::move(meshFilter);
    item.transform = std::move(transform);
    item.insertionIndex = m_insertionCounter++;

    // 从 Widget 获取 displayLayer（-10 ~ 10）
    if (item.transform) {
        auto* widget = item.transform->getGameObject();
        if (widget) {
            item.displayLayer = widget->getDisplayLayer();
        }
    }

    m_renderables.push_back(std::move(item));
}

// ---------------------------------------------------------------------------
// 渲染阶段：增量检查 → 需要时重建批次 → 渲染
// ---------------------------------------------------------------------------
void BatchManager::renderBatches(std::shared_ptr<FrameState> frameState) {
    auto& statistics = frameState->batchStatistics;
    statistics.renderItemCount = static_cast<uint32_t>(m_renderables.size());

    // ---- 增量合批：比较可渲染列表是否与上一帧相同 ----
    if (isRenderableListUnchanged()) {
        ++statistics.cacheHitCount;
    } else {
        ++statistics.cacheMissCount;
        buildBatches(statistics);
        m_batchesDirty = false;
    }

    statistics.batchCount = static_cast<uint32_t>(m_batches.size());

    // ---- UBO 更新 ----
    m_ubo->update(frameState);
    Matrix4 projectionMatrix = frameState->camera->getProjectionView();
    const uint32_t drawCallsBeforeBatches = frameState->drawCallCount;

    // ---- 渲染所有批次 ----
    for (auto& batch : m_batches) {
        if (batch.materials.empty()) continue;

        if (frameState->isSSBOSupport && batch.isSSBOShader && batch.materials.size() > 1) {
            ++statistics.ssboBatchCount;
            renderSSBOBatch(frameState, batch);
        } else {
            ++statistics.standardBatchCount;
            if (batch.isSSBOShader && batch.materials.size() > 1 && !frameState->isSSBOSupport) {
                ++statistics.ssboFallbackBatchCount;
            }
            renderStandardBatch(frameState, batch, projectionMatrix);
        }
    }
    statistics.batchDrawCallCount = frameState->drawCallCount - drawCallsBeforeBatches;
}

// ---------------------------------------------------------------------------
// 清空：归还批次池，交换可渲染列表以支持增量比对
// ---------------------------------------------------------------------------
void BatchManager::clear() {
    // 保留本帧可渲染列表用于下一帧增量比对
    m_prevRenderables.swap(m_renderables);
    m_renderables.clear();
    m_insertionCounter = 0;
}


// ===================================================================
// 内部实现
// ===================================================================

uint64_t BatchManager::computeMaterialKey(const std::shared_ptr<Material>& material) {
    if (!material) return 0ull;

    // 组合 shader 名称哈希 + 主纹理指针作为排序键。
    // 注意：这只是排序用的粗粒度 key，真正合批仍须调用 Material::isEqual。
    // 同一 shader + 同一纹理 = 可合批
    uint64_t shaderHash = std::hash<std::string>{}(material->getShaderName());
    uint64_t texPtr = 0ull;
    auto tex = material->getTexture("texture");
    if (!tex) {
        tex = material->getTexture("u_texture");
    }
    if (!tex) {
        tex = material->getTexture("mainTexture");
    }
    if (!tex) {
        tex = material->getTexture("diffuseMap");
    }
    if (tex) {
        texPtr = reinterpret_cast<uint64_t>(tex.get());
    }

    return shaderHash ^ (texPtr << 7);
}

bool BatchManager::isRenderableListUnchanged() const {
    if (m_batchesDirty) return false;
    if (m_renderables.size() != m_prevRenderables.size()) return false;

    for (size_t i = 0; i < m_renderables.size(); ++i) {
        if (!m_renderables[i].isSameRenderableAs(m_prevRenderables[i])) {
            return false;
        }
    }
    return true;
}

BatchBreakReason BatchManager::getBreakReason(const RenderableItem& previous,
                                               const RenderableItem& current) {
    if (previous.displayLayer != current.displayLayer) {
        return BatchBreakReason::DisplayLayer;
    }
    if (previous.material->getShaderName() != current.material->getShaderName()) {
        return BatchBreakReason::Shader;
    }
    if (computeMaterialKey(previous.material) != computeMaterialKey(current.material)) {
        return BatchBreakReason::Texture;
    }
    if (!previous.material->isEqual(current.material)) {
        return BatchBreakReason::MaterialState;
    }
    return BatchBreakReason::OrderBarrier;
}

void BatchManager::buildBatches(BatchStatistics& statistics) {
    // 归还旧批次（buildBatches 前确保池中批次被回收）
    RenderBatchPool::getInstance().releaseAll(m_batches);

    if (m_renderables.empty()) return;

    // ---- 步骤 1：按 (displayLayer, materialKey, insertionIndex) 排序 ----
    // displayLayer 为主排序键（保证跨层 Z-order 正确）
    // materialKey 为次排序键（将同材质聚拢以最大化合批）
    // insertionIndex 为第三排序键（保持同材质内的 Z-order 稳定）
    // 不要直接排序 m_renderables：它还要按 Widget 遍历顺序与下一帧比较。
    // 若原地排序，材质交错场景会导致增量检查每帧都误判为变化。
    auto sortedRenderables = m_renderables;
    std::stable_sort(sortedRenderables.begin(), sortedRenderables.end(),
        [](const RenderableItem& a, const RenderableItem& b) {
            if (a.displayLayer != b.displayLayer) {
                return a.displayLayer < b.displayLayer;
            }
            uint64_t keyA = computeMaterialKey(a.material);
            uint64_t keyB = computeMaterialKey(b.material);
            if (keyA != keyB) {
                return keyA < keyB;
            }
            return a.insertionIndex < b.insertionIndex;
        });

    // ---- 步骤 2：将连续同材质项合并为批次 ----
    uint64_t currentKey = 0;
    const RenderableItem* previousItem = nullptr;
    for (auto& item : sortedRenderables) {
        uint64_t itemKey = computeMaterialKey(item.material);

        // 尝试合并到最后一个批次
        if (!m_batches.empty()) {
            auto& lastBatch = m_batches.back();
            if (!lastBatch.materials.empty() &&
                currentKey == itemKey &&
                lastBatch.materials.front()->isEqual(item.material)) {
                lastBatch.materials.emplace_back(item.material);
                lastBatch.meshFilters.emplace_back(item.meshFilter);
                lastBatch.transforms.emplace_back(item.transform);
                previousItem = &item;
                continue;
            }
        }

        // 新建批次（从对象池获取）
        if (previousItem) {
            statistics.recordBreak(getBreakReason(*previousItem, item));
        }
        currentKey = itemKey;
        RenderBatch newBatch = RenderBatchPool::getInstance().acquire(
            item.material->getShaderName(), item.material->isSSBOShader());
        newBatch.materials.emplace_back(item.material);
        newBatch.meshFilters.emplace_back(item.meshFilter);
        newBatch.transforms.emplace_back(item.transform);
        m_batches.emplace_back(std::move(newBatch));
        previousItem = &item;
    }

    m_batchesDirty = false;
}

// ---------------------------------------------------------------------------
// SSBO 实例化渲染路径
// ---------------------------------------------------------------------------
void BatchManager::renderSSBOBatch(std::shared_ptr<FrameState> frameState, RenderBatch& batch) {
    auto ssboManager = frameState->ssboManager;
    ssboManager->updateSSBOForShader(batch.shaderName, batch);

    auto material = batch.materials[0];
    material->applyBatch();
    batch.vertexArray->updateFromMeshes(frameState, material->getBatchShader(), batch.meshFilters);

    m_ubo->bind(material->getBatchShader());
    batch.vertexArray->draw(frameState);
}

// ---------------------------------------------------------------------------
// 标准逐对象渲染路径
// ---------------------------------------------------------------------------
void BatchManager::renderStandardBatch(std::shared_ptr<FrameState> frameState, RenderBatch& batch,
                                        Matrix4& projectionMatrix) {
    for (size_t index = 0; index < batch.materials.size(); index++) {
        auto material = batch.materials[index];
        Matrix4 modelMatrix = batch.transforms[index]->getWorldMatrix();
        if (batch.isSSBOShader) {
            material->setMatrix4("model", modelMatrix);
        } else {
            material->setMatrix4("mvp", projectionMatrix * modelMatrix);
        }
        material->apply();

        auto meshFilter = batch.meshFilters[index];
        auto meshRenderer = meshFilter->getComponent<MeshRenderer>();
        meshRenderer->getVertexArray()->updateFromMeshes(
            frameState, material->getShader(), {batch.meshFilters[index]});
        if (batch.isSSBOShader) {
            m_ubo->bind(material->getShader());
        }
        meshRenderer->getVertexArray()->draw(frameState);
    }
}

} // namespace morrow
