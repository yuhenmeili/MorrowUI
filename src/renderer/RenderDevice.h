//
// Created by lance on 2023/12/8.
//

#ifndef MORROW_RENDERER_ESDEVICE_H_
#define MORROW_RENDERER_ESDEVICE_H_

#include "GPUBufferDevice.h"
#include "GPUTextureDevice.h"
#include "GPUShaderDevice.h"
#include "GPURenderPassDevice.h"

namespace morrow {

// ---------------------------------------------------------------------------
// RenderDevice — 统一的 GPU 抽象接口
//
// 通过多重继承组合四个子接口：
//   GPUBufferDevice     — VBO / UBO / SSBO / Fence
//   GPUTextureDevice    — Texture2D
//   GPUShaderDevice     — GPUProgram (Shader) / Uniform
//   GPURenderPassDevice — FBO / 渲染状态 / 平台上下文
//
// 新代码可按需依赖子接口以降低耦合；旧代码使用 RenderDevice* 不受影响。
// ---------------------------------------------------------------------------
class RenderDevice : public GPUBufferDevice,
                     public GPUTextureDevice,
                     public GPUShaderDevice,
                     public GPURenderPassDevice {
public:
    ~RenderDevice() override = default;
};

} // namespace morrow

#endif //MORROW_RENDERER_ESDEVICE_H_
