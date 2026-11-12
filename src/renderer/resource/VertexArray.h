//
// Created by lance on 2023/12/4.
//

#ifndef MORROW_RENDERER_VERTEXARRAY_H_
#define MORROW_RENDERER_VERTEXARRAY_H_

#include <cstdint>
#include <memory>
#include <vector>
#include "GpuTypes.h"    // HwVBO, HwGPUProgram（无需拉入完整设备接口）

namespace morrow {
struct FrameState;
class Mesh;
class MeshFilter;
using namespace Math;

class VertexArray {
public:
    ~VertexArray();

    void updateFromMeshes(std::shared_ptr<FrameState> frameState, HwGPUProgram program, const std::vector<std::shared_ptr<MeshFilter>>& meshFilters);

    void draw(std::shared_ptr<FrameState> frameState, int32_t instanceCount = 1);

    /// 清空网格上传缓存：池化复用的 VertexArray 归还 RenderBatchPool 时必须
    /// 调用。同池 key（shaderName|isSSBO）的多个池对象会在逻辑批次间轮换
    /// （LIFO），下一任持有者若因缓存命中跳过上传，会画出上一任逻辑批次的
    /// 旧顶点数据；weak_ptr 指向的 Mesh 释放后地址被新 Mesh 复用且 revision
    /// 巧合相同时，缓存判定还会误命中（多屏 Independent 双窗口共享全局池时
    /// 高频轮换，表现为新旧内容交替闪烁）
    void invalidateMeshCache() { m_uploadedMeshes.clear(); }

private:
    struct UploadedMeshState {
        std::weak_ptr<Mesh> mesh;
        uint64_t revision = 0;
    };

    bool needsMeshUpload(HwGPUProgram program, const std::vector<std::shared_ptr<MeshFilter>>& meshFilters) const;

    void recordUploadedMeshes(HwGPUProgram program, const std::vector<std::shared_ptr<MeshFilter>>& meshFilters);

    HwVBO m_vbo{0};
    HwVBO m_instanceVBO{0};
    HwGPUProgram m_uploadedProgram{0};
    std::vector<UploadedMeshState> m_uploadedMeshes;
};

using VertexArraySharedPtr = std::shared_ptr<VertexArray>;
}


#endif //MORROW_RENDERER_VERTEXARRAY_H_