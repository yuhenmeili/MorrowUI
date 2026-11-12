//
// Created by 0060328 on 26-6-8.
//

#include "RenderBatchPool.h"
#include "ssbo/ShaderStorageBuffer.h"
#include "VertexArray.h"

namespace morrow {

std::string RenderBatchPool::makeKey(const std::string& shaderName, bool isSSBOShader) {
    return shaderName + "|" + (isSSBOShader ? "1" : "0");
}

RenderBatch RenderBatchPool::acquire(const std::string& shaderName, bool isSSBOShader) {
    std::string key = makeKey(shaderName, isSSBOShader);
    auto it = m_pool.find(key);
    if (it != m_pool.end() && !it->second.empty()) {
        // 池中命中：vertexArray/ssbo 均已在首次创建时初始化，直接复用
        RenderBatch batch = std::move(it->second.back());
        it->second.pop_back();
        return batch;
    }
    // 池中无缓存：新建并完整初始化所有可复用资源
    RenderBatch batch;
    batch.shaderName = shaderName;
    batch.isSSBOShader = isSSBOShader;
    batch.vertexArray = std::make_shared<VertexArray>();
    batch.ssbo = std::make_shared<ShaderStorageBuffer>();
    return batch;
}

void RenderBatchPool::release(RenderBatch&& batch) {
    if (batch.materials.empty() && batch.meshFilters.empty() && batch.transforms.empty()) {
        return; // 空批次直接丢弃，不缓存
    }
    // 归还前清空动态数据，保留 vertexArray/ssbo 等可复用资源
    batch.materials.clear();
    batch.meshFilters.clear();
    batch.transforms.clear();
    batch.ssboLayout.reset();
    batch.clipRect = {};
    // 同 key 的多个池对象会在逻辑批次间轮换（LIFO），归还时必须作废其
    // VertexArray 的网格缓存，否则下一帧的持有者会因缓存命中跳过上传，
    // 画出该对象上一任逻辑批次的旧顶点数据（新旧内容交替闪烁）
    if (batch.vertexArray) {
        batch.vertexArray->invalidateMeshCache();
    }

    std::string key = makeKey(batch.shaderName, batch.isSSBOShader);
    m_pool[key].emplace_back(std::move(batch));
}

void RenderBatchPool::releaseAll(std::vector<RenderBatch>& batches) {
    for (auto& batch : batches) {
        // 归还前清空动态数据
        batch.materials.clear();
        batch.meshFilters.clear();
        batch.transforms.clear();
        batch.ssboLayout.reset();
        batch.clipRect = {};
        // 同 release：轮换复用前必须作废网格缓存（buildBatches 归还后立即
        // 按 新的可渲染列表 重新 acquire，不作废会命中上一任批次的旧数据）
        if (batch.vertexArray) {
            batch.vertexArray->invalidateMeshCache();
        }

        std::string key = makeKey(batch.shaderName, batch.isSSBOShader);
        m_pool[key].emplace_back(std::move(batch));
    }
    batches.clear();
}

void RenderBatchPool::clear() {
    m_pool.clear();
}

} // namespace morrow
