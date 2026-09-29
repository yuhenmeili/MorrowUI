//
// GPUTextureDevice.h — GPU 纹理设备接口
//
// 职责：Texture2D 的创建、更新、绑定、格式查询。
// 这是从 RenderDevice 拆分出的子接口，可按需单独依赖。
//

#ifndef MORROW_RENDERER_GPUTEXTUREDEVICE_H_
#define MORROW_RENDERER_GPUTEXTUREDEVICE_H_

#include <cstdint>

#include "GpuTypes.h"

namespace morrow {

class GPUTextureDevice {
public:
    virtual ~GPUTextureDevice() = default;

    //---------------------------------------------------Texture2D---------------------------------------------------
    virtual HwTexture2D createTexture2D(ImageType imageType) = 0;

    virtual void deleteTexture2D(HwTexture2D texture) = 0;

    virtual void useTexture2D(HwTexture2D texture, uint32_t index) = 0;

    virtual bool isTextureFormatSupported(PixelDataFormat textureFormat) = 0;

    virtual void updateTexture2D(HwTexture2D texture, const TextureData& data) = 0;

    virtual void updateSubTexture2D(HwTexture2D texture, const TextureData& data,
                                    int32_t x, int32_t y, int32_t width, int32_t height,
                                    const unsigned char* sourceData) = 0;
};

} // namespace morrow

#endif // MORROW_RENDERER_GPUTEXTUREDEVICE_H_
