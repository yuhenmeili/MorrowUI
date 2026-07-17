//
// GPUBufferDevice.h — GPU 缓冲区设备接口
//
// 职责：VBO / UBO / SSBO 的创建、更新、绑定、绘制，以及 GPU Fence 资源回收。
// 这是从 RenderDevice 拆分出的子接口，可按需单独依赖。
//

#ifndef MORROW_RENDERER_GPUBUFFERDEVICE_H_
#define MORROW_RENDERER_GPUBUFFERDEVICE_H_

#include <cstdint>
#include <memory>

#include "GpuTypes.h"

namespace morrow {

class GPUBufferDevice {
public:
    virtual ~GPUBufferDevice() = default;

    //---------------------------------------------------VBO---------------------------------------------------
    virtual HwVBO createVBO() = 0;

    virtual void updateVBO(HwGPUProgram program, HwVBO vbo, VBODataSharedPtr vboData) = 0;

    virtual void deleteVBO(HwVBO vbo) = 0;

    virtual void drawVBO(HwVBO vbo, int32_t instanceCount) = 0;

    //---------------------------------------------------UBO---------------------------------------------------
    virtual HwUBO createUBO() = 0;

    virtual void updateUBO(HwUBO ubo, std::shared_ptr<UBOData> uboData) = 0;

    virtual void bindUBO(HwGPUProgram program, HwUBO ubo, const std::string& blockName, uint32_t bindingPoint) = 0;

    //---------------------------------------------------SSBO---------------------------------------------------
    virtual HwSSBO createSSBO() = 0;

    virtual void updateSSBO(HwSSBO ssbo, std::shared_ptr<SSBOData> ssboData, uint32_t bindingPoint) = 0;

    //---------------------------------------------------GPU Fence（用于资源回收）---------------------------------------------------
    /// 在 GPU 命令流中插入 fence，返回不透明句柄（GL 下为 GLsync）。
    virtual void* insertFence() { return nullptr; }

    /// 等待 fence 完成，timeoutNs 为 0 表示非阻塞。返回 true 表示已完成可安全回收。
    virtual bool waitFence(void* fence, uint64_t timeoutNs) { return false; }

    virtual void deleteFence(void* fence) {}
};

} // namespace morrow

#endif // MORROW_RENDERER_GPUBUFFERDEVICE_H_
