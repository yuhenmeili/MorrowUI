//
// Created by lance on 2023/12/4.
//

#ifndef MORROW_RENDERER_VERTEXARRAY_H_
#define MORROW_RENDERER_VERTEXARRAY_H_

#include <cstdint>
#include <memory>
#include <vector>
#include "GpuTypes.h"    // VBO, GPUProgram（无需拉入完整设备接口）

#ifdef OPENGL_GLFW
#include "platform/wgl/OpenglHeader.h"
#else
#include "platform/egl/GLESHeader.h"
#endif

namespace morrow {
struct FrameState;
class Mesh;
class MeshFilter;
using namespace Math;

class VertexArray {
public:
    ~VertexArray();

    void updateFromMeshes(std::shared_ptr<FrameState> frameState, GPUProgram* program, const std::vector<std::shared_ptr<MeshFilter>>& meshFilters);

    void draw(std::shared_ptr<FrameState> frameState, int32_t instanceCount = 1);

private:
    struct UploadedMeshState {
        std::weak_ptr<Mesh> mesh;
        uint64_t revision = 0;
    };

    bool needsMeshUpload(GPUProgram* program, const std::vector<std::shared_ptr<MeshFilter>>& meshFilters) const;

    void recordUploadedMeshes(GPUProgram* program, const std::vector<std::shared_ptr<MeshFilter>>& meshFilters);

    VBO* m_vbo = nullptr;

    VBO* m_instanceVBO = nullptr; // 实例化数据VBO
    GPUProgram* m_uploadedProgram = nullptr;
    std::vector<UploadedMeshState> m_uploadedMeshes;
};

using VertexArraySharedPtr = std::shared_ptr<VertexArray>;
}


#endif //MORROW_RENDERER_VERTEXARRAY_H_