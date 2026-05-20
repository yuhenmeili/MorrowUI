//
// Created by lance on 2023/12/4.
//

#ifndef MORROW_RENDERER_VERTEXARRAY_H_
#define MORROW_RENDERER_VERTEXARRAY_H_

#include <memory>
#include "GpuTypes.h"    // VBO, GPUProgram（无需拉入完整设备接口）

#ifdef OPENGL_GLFW
#include "../platform/wgl/OpenglHeader.h"
#else
#include "../platform/egl/GLESHeader.h"
#endif

namespace morrow {
struct FrameState;
using namespace Math;

class VertexArray {
public:
    ~VertexArray();

    void updateFromMeshes(std::shared_ptr<FrameState> frameState, GPUProgram* program, const std::vector<std::shared_ptr<MeshFilter>>& meshFilters);

    void draw(std::shared_ptr<FrameState> frameState, int32_t instanceCount = 1);

private:
    VBO* m_vbo = nullptr;

    VBO* m_instanceVBO = nullptr; // 实例化数据VBO
};

using VertexArraySharedPtr = std::shared_ptr<VertexArray>;
}


#endif //MORROW_RENDERER_VERTEXARRAY_H_
