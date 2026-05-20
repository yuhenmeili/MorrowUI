//
// Created by 0060328 on 25-10-20.
//

#include "BatchManager.h"

#include "BatchDataDefine.h"
#include "Material.h"
#include "OrthographicCamera.h"
#include "SSBOManager.h"
#include "UniformBuffer.h"
#include "VertexArray.h"
#include "base/MeshFilter.h"
#include "base/MeshRenderer.h"
#include "base/Component.inl"
#include "base/Transform.h"

namespace morrow {
BatchManager::BatchManager() {
    m_ubo = std::make_shared<UniformBuffer>();
}

void BatchManager::addRenderable(std::shared_ptr<Material> material, std::shared_ptr<MeshFilter> meshFilter, std::shared_ptr<Transform> transform) {
    // 仅合并连续出现的相同材质，避免跨层级/跨控件重排导致后绘制的背景压住先后关系中的文字。
    if (!m_batches.empty()) {
        auto& lastBatch = m_batches.back();
        if (!lastBatch.materials.empty() && lastBatch.materials.front()->isEqual(material)) {
            lastBatch.materials.emplace_back(material);
            lastBatch.meshFilters.emplace_back(meshFilter);
            lastBatch.transforms.emplace_back(transform);
            return;
        }
    }

    // 创建新的批次
    RenderBatch newBatch;
    newBatch.shaderName = material->getShaderName();
    newBatch.isSSBOShader = material->isSSBOShader();
    newBatch.vertexArray = std::make_shared<VertexArray>();
    newBatch.ssbo = std::make_shared<ShaderStorageBuffer>();
    newBatch.materials.push_back(material);
    newBatch.meshFilters.emplace_back(meshFilter);
    newBatch.transforms.push_back(transform);
    m_batches.emplace_back(newBatch);
}

void BatchManager::renderBatches(std::shared_ptr<FrameState> frameState) {
    m_ubo->update(frameState);

    Matrix4 projectionMatrix = frameState->camera->getProjectionView();

    for (auto& batch : m_batches) {
        if (batch.materials.empty()) continue;
        if (frameState->isSSBOSupport && batch.isSSBOShader && batch.materials.size() > 1) {
            renderSSBOBatch(frameState, batch);
        } else {
            renderStandardBatch(frameState, batch, projectionMatrix);
        }
    }
}

void BatchManager::clear() {
    for (auto& batch : m_batches) {
        // 添加到现有批次
        batch.materials.clear();
        batch.meshFilters.clear();
        batch.transforms.clear();
    }
}

void BatchManager::renderSSBOBatch(std::shared_ptr<FrameState> frameState, RenderBatch& batch) {
    auto ssboManager = frameState->ssboManager;
    ssboManager->updateSSBOForShader(batch.shaderName, batch);

    // 使用实例化渲染
    auto material = batch.materials[0];
    material->applyBatch();
    batch.vertexArray->updateFromMeshes(frameState, material->getBatchShader(), batch.meshFilters);

    m_ubo->bind(material->getBatchShader());
    batch.vertexArray->draw(frameState);
}

void BatchManager::renderStandardBatch(std::shared_ptr<FrameState> frameState, RenderBatch& batch, Matrix4& projectionMatrix) {
    for (int index = 0; index < batch.materials.size(); index++) {
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
        meshRenderer->getVertexArray()->updateFromMeshes(frameState, material->getShader(), {batch.meshFilters[index]});
        if (batch.isSSBOShader) {
            m_ubo->bind(material->getShader());
        }
        meshRenderer->getVertexArray()->draw(frameState);
    }
}
} // morrow
