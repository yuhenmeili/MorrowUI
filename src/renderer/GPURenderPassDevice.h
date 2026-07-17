//
// GPURenderPassDevice.h — GPU 渲染通道设备接口
//
// 职责：FBO 管理、渲染状态（Blend/Depth/Cull/Viewport/Clear）、平台上下文管理。
// 这是从 RenderDevice 拆分出的子接口，可按需单独依赖。
//

#ifndef MORROW_RENDERER_GPURENDERPASSDEVICE_H_
#define MORROW_RENDERER_GPURENDERPASSDEVICE_H_

#include <cstdint>

#include "GpuTypes.h"

namespace morrow {

class GPURenderPassDevice {
public:
    virtual ~GPURenderPassDevice() = default;

    //---------------------------------------------------平台/上下文---------------------------------------------------
    virtual void makeCurrent(void* window) = 0;

    virtual void present(void* window) = 0;

    virtual void debugDriver() = 0;

    //---------------------------------------------------清除与视口---------------------------------------------------
    virtual void clear() = 0;

    virtual void setClearColor(float r, float g, float b, float alpha) = 0;

    virtual void setViewPort(int32_t x, int32_t y, int32_t width, int32_t height) = 0;

    virtual void dumpFrameBuffer(int32_t x, int32_t y, int32_t displayWidth, int32_t displayHeight,
                                 int32_t rectX, int32_t rectY, int32_t rectWidth, int32_t rectHeight,
                                 int32_t comp) = 0;

    virtual bool checkSSBOSupport() = 0;

    //---------------------------------------------------Blend---------------------------------------------------
    virtual void enableBlend() = 0;

    virtual void disableBlend() = 0;

    //---------------------------------------------------深度/剔除---------------------------------------------------
    virtual void setDepthTest(bool enable) = 0;

    virtual void setDepthWrite(bool enable) = 0;

    virtual void setCullFace(CullFaceMode mode) = 0;

    virtual void clearDepth() = 0;

    //---------------------------------------------------FBO---------------------------------------------------
    virtual HwRenderTarget createRenderTarget(int32_t w, int32_t h,
                                              HwTexture2D* outColorTexture = nullptr) = 0;

    virtual void deleteRenderTarget(HwRenderTarget rt) = 0;

    virtual void bindRenderTarget(HwRenderTarget rt) = 0;

    virtual void unbindRenderTarget() = 0;
};

} // namespace morrow

#endif // MORROW_RENDERER_GPURENDERPASSDEVICE_H_
