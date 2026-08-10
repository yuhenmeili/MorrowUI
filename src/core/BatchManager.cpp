//
// Created by 0060328 on 25-10-20.
//

#include "BatchManager.h"

#include "BatchDataDefine.h"
#include "Material.h"
#include "OrthographicCamera.h"
#include "RenderBatchPool.h"
#include "ssbo/SSBOManager.h"
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
                                 std::shared_ptr<Transform> transform,
                                 bool underlay) {
    RenderItem item;
    item.material = std::move(material);
    item.meshFilter = std::move(meshFilter);
    item.transform = std::move(transform);
    item.batchKey = createBatchKey(item.material, item.meshFilter);
    item.insertionIndex = m_insertionCounter++;

    // 从 Widget 获取 displayLayer（-10 ~ 10）
    if (item.transform) {
        auto* widget = item.transform->getGameObject();
        if (widget) {
            item.displayLayer = widget->getDisplayLayer();
        }
    }
    if (underlay) {
        m_underlayRenderables.push_back(std::move(item));
    } else {
        m_renderables.push_back(std::move(item));
    }
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
                renderNonSSBOFallback(frameState, batch, projectionMatrix);
            } else {
                renderStandardBatch(frameState, batch, projectionMatrix);
            }
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
    m_prevUnderlayRenderables.swap(m_underlayRenderables);
    m_underlayRenderables.clear();
    m_insertionCounter = 0;
}


// ===================================================================
// 内部实现
// ===================================================================

BatchCompatibilityKey BatchManager::createBatchKey(const std::shared_ptr<Material>& material, const std::shared_ptr<MeshFilter>& meshFilter) {
    BatchCompatibilityKey key;
    if (!material) return key;

    key.materialStateHash = material->getBatchCompatibilityHash();
    if (meshFilter && meshFilter->getMesh()) {
        key.meshStateHash = meshFilter->getMesh()->getBatchCompatibilityHash();
    }
    return key;
}

bool BatchManager::isRenderableListUnchanged() const {
    if (m_batchesDirty) return false;
    if (!BatchBuilder::areRenderItemListsEquivalent(m_underlayRenderables, m_prevUnderlayRenderables)) {
        return false;
    }
    return BatchBuilder::areRenderItemListsEquivalent(m_renderables, m_prevRenderables);
}

void BatchManager::buildBatches(BatchStatistics& statistics) {
    RenderBatchPool::getInstance().releaseAll(m_batches);
    // underlay（阴影等底层效果）先合批，保证绘制顺序在前
    buildFromItems(m_underlayRenderables, statistics);
    buildFromItems(m_renderables, statistics);
    m_batchesDirty = false;
}

void BatchManager::buildFromItems(const std::vector<RenderItem>& items, BatchStatistics& statistics) {
    if (items.empty()) return;

    const BatchBuildResult result = m_batchBuilder.build(items);
    for (const auto reason : result.breakReasons) {
        statistics.recordBreak(reason);
    }

    m_batches.reserve(m_batches.size() + result.groups.size());
    for (const auto& group : result.groups) {
        if (group.itemIndices.empty()) continue;
        const auto& firstItem = items[group.itemIndices.front()];
        RenderBatch newBatch = RenderBatchPool::getInstance().acquire(
            firstItem.material->getShaderName(),
            firstItem.material->isSSBOShader());
        newBatch.ssboLayout = firstItem.material->getSSBOLayout();

        for (const uint32_t itemIndex : group.itemIndices) {
            const auto& item = items[itemIndex];
            newBatch.materials.emplace_back(item.material);
            newBatch.meshFilters.emplace_back(item.meshFilter);
            newBatch.transforms.emplace_back(item.transform);
        }
        m_batches.emplace_back(std::move(newBatch));
    }
}

// ---------------------------------------------------------------------------
// SSBO 实例化渲染路径
// ---------------------------------------------------------------------------
void BatchManager::renderSSBOBatch(std::shared_ptr<FrameState> frameState, RenderBatch& batch) {
    auto ssboManager = frameState->ssboManager;
    ssboManager->updateSSBO(batch);

    auto material = batch.materials[0];
    material->applyBatch();
    batch.vertexArray->updateFromMeshes(frameState, material->getBatchShader(), batch.meshFilters);

    m_ubo->bind(material->getBatchShader());
    batch.vertexArray->draw(frameState);
}

// ---------------------------------------------------------------------------
// 标准逐对象渲染路径
// ---------------------------------------------------------------------------
void BatchManager::renderStandardBatch(std::shared_ptr<FrameState> frameState, RenderBatch& batch, Matrix4& projectionMatrix) {
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
        if (!meshRenderer) {
            continue;
        }
        meshRenderer->getVertexArray()->updateFromMeshes(frameState, material->getShader(), {batch.meshFilters[index]});
        if (batch.isSSBOShader) {
            m_ubo->bind(material->getShader());
        }
        meshRenderer->getVertexArray()->draw(frameState);
    }
}

void BatchManager::renderNonSSBOFallback(std::shared_ptr<FrameState> frameState, RenderBatch& batch, Matrix4& projectionMatrix) {
    renderStandardBatch(frameState, batch, projectionMatrix);
}
} // namespace morrow
