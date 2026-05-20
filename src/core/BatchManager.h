//
// Created by 0060328 on 25-10-20.
//

#ifndef BATCHMANAGER_H
#define BATCHMANAGER_H
#include <memory>
#include <vector>
#include "BatchDataDefine.h"

namespace morrow {
struct FrameState;
class Transform;
class Mesh;
class Material;
class UniformBuffer;

class BatchManager {
public:
    BatchManager();

    // 添加渲染对象到批处理队列
    void addRenderable(std::shared_ptr<Material> material, std::shared_ptr<MeshFilter> meshFilter, std::shared_ptr<Transform> transform);

    // 执行批处理渲染
    void renderBatches(std::shared_ptr<FrameState> frameState);

    // 清空批处理队列
    void clear();

private:
    void renderSSBOBatch(std::shared_ptr<FrameState> frameState, RenderBatch& batch);

    void renderStandardBatch(std::shared_ptr<FrameState> frameState, RenderBatch& batch, Matrix4& projectionMatrix);
    std::vector<RenderBatch> m_batches;
    std::shared_ptr<UniformBuffer> m_ubo;
};
} // morrow

#endif //BATCHMANAGER_H
