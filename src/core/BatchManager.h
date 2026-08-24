//
// Created by 0060328 on 25-10-20.
//

#ifndef BATCHMANAGER_H
#define BATCHMANAGER_H
#include <cstdint>
#include <memory>
#include <vector>
#include "BatchBuilder.h"
#include "BatchDataDefine.h"
#include "renderer/BatchStatistics.h"

namespace morrow {
struct FrameState;
class Transform;
class Mesh;
class Material;
class UniformBuffer;

// ---------------------------------------------------------------------------
// BatchManager
//
// 优化点：
//   1. Z-order 敏感的排序合批 — 按 (displayLayer, materialKey) 排序后合并，
//      大幅减少同层内材质交错导致的冗余批次。
//   2. 增量合批 — 帧间比较可渲染列表，未变化时直接复用上一帧的批次结构，
//      避免每帧重建带来的 CPU 开销。
// ---------------------------------------------------------------------------
class BatchManager {
public:
    BatchManager();

    /// 收集可渲染对象（Widget 树遍历时每帧调用）
    /// @param underlay 为 true 时该渲染项排在普通项之前绘制（如阴影等底层效果）
    void addRenderable(std::shared_ptr<Material> material,
                       std::shared_ptr<MeshFilter> meshFilter,
                       std::shared_ptr<Transform> transform,
                       const ClipRect& clipRect,
                       bool underlay = false);

    /// 执行合批渲染（在 commitRenderPass 中调用）
    void renderBatches(std::shared_ptr<FrameState> frameState);

    /// 清空本帧数据，归还批次到对象池
    void clear();

private:
    // ---- 内部方法 ----

    /// 调用纯逻辑 BatchBuilder，并将结果物化为带 GPU 资源的 RenderBatch
    void buildBatches(BatchStatistics& statistics);

    /// 将一组 RenderItem 合批并物化为 RenderBatch，追加到 m_batches
    void buildFromItems(const std::vector<RenderItem>& items, BatchStatistics& statistics);

    /// 检查本帧可渲染列表是否与上一帧相同（增量合批判断）
    bool isRenderableListUnchanged() const;

    static BatchCompatibilityKey createBatchKey(const std::shared_ptr<Material>& material,const std::shared_ptr<MeshFilter>& meshFilter);

    // SSBO 路径渲染
    void renderSSBOBatch(std::shared_ptr<FrameState> frameState, RenderBatch& batch);

    // 标准路径渲染
    void renderStandardBatch(std::shared_ptr<FrameState> frameState, RenderBatch& batch, Matrix4& projectionMatrix);

    /// 安全 fallback：当前非 SSBO shader 的 per-object uniform 无法一次 draw 表达，
    /// 因此保持批次顺序并逐对象提交，避免错误烘焙变换/圆角/透明度。
    void renderNonSSBOFallback(std::shared_ptr<FrameState> frameState, RenderBatch& batch, Matrix4& projectionMatrix);

    void applyClipRect(const std::shared_ptr<FrameState>& frameState, const ClipRect& clipRect);

    // ---- 数据成员 ----

    std::vector<RenderItem> m_renderables; // 本帧收集（普通渲染项）
    std::vector<RenderItem> m_prevRenderables; // 上一帧（增量比对）
    std::vector<RenderItem> m_underlayRenderables; // 本帧收集（底层渲染项，先绘制）
    std::vector<RenderItem> m_prevUnderlayRenderables; // 上一帧（增量比对）
    std::vector<RenderBatch> m_batches; // 构建的批次
    BatchBuilder m_batchBuilder;
    bool m_batchesDirty = true; // 本帧是否需要重建批次
    uint32_t m_insertionCounter = 0; // 插入序号（每帧重置）

    std::shared_ptr<UniformBuffer> m_ubo;
};
} // morrow

#endif //BATCHMANAGER_H
