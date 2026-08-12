//
// Created by lance on 2022/10/11.
//

#ifndef MORROW_TEXTURE_H
#define MORROW_TEXTURE_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "DriverEnums.h"
#include "FrameState.h"
#include "GlobalDefine.h"
#include "GpuTypes.h"
#include "basis_universal/transcoder/basisu_containers.h"
#include "debug/ObjectRegistry.h"

namespace morrow {
struct TextureInfo {
    std::string textureName;

    std::string imageUrl;
    std::shared_ptr<unsigned char> textureDataSharedPtr;
    std::shared_ptr<std::vector<unsigned char>> textureDataBuffer;
    void* textureDataRawPtr = nullptr;
    basisu::vector<uint8_t> basisData;

    int32_t imageWidth;
    int32_t imageHeight;
    PixelDataFormat format = PixelDataFormat::RGBA;
    int32_t bytes = 0;
    bool compressedTexture = false;
    SamplerMinFilter minFilterType = SamplerMinFilter::LINEAR;
    SamplerMagFilter magFilterType = SamplerMagFilter::LINEAR;
    ImageType imageType = ImageType::IMAGE;
    std::function<void()> gpuUseCompleteCallback;

    bool textureNeedUpLoad = true;
};

using TextureInfoSharedPtr = std::shared_ptr<TextureInfo>;

class Texture : public std::enable_shared_from_this<Texture> {
public:
    static std::shared_ptr<Texture> create(ImageType imageType = ImageType::IMAGE);

    virtual ~Texture();

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~options~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    Texture& setImageUrl(const std::string& imageUrl);

    Texture& setTextureData(std::shared_ptr<unsigned char> textureData, int32_t imageWidth, int32_t imageHeight, PixelDataFormat format = PixelDataFormat::RGBA, int32_t bytes = 0,
                            bool compressedTexture = false);

    Texture& setTextureData(std::shared_ptr<std::vector<unsigned char>> textureData, int32_t imageWidth, int32_t imageHeight, PixelDataFormat format = PixelDataFormat::RGBA,
                            bool compressedTexture = false);

    Texture& setTextureData(void* textureData, int32_t imageWidth, int32_t imageHeight, PixelDataFormat format = PixelDataFormat::RGBA, int32_t bytes = 0,
                            bool compressedTexture = false);

    Texture& setOESTextureData(void* textureData, int32_t imageWidth, int32_t imageHeight, PixelDataFormat format = PixelDataFormat::RGBA, int32_t bytes = 0,
                               std::function<void()> gpuUseCompleteCallback = {});

    Texture& setTextureName(std::string textureName);

    Texture& setFormat(PixelDataFormat format);

    Texture& setMinFilterType(SamplerMinFilter minFilterType);

    Texture& setMagFilterType(SamplerMagFilter magFilterType);

    Texture& setWidth(int32_t width);

    Texture& setHeight(int32_t height);

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~options~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ImageType getImageType() const;

    int32_t getWidth();

    int32_t getHeight();

    uint64_t getRevision() const;

    std::string getImageInfo();

    void prepare();

    virtual void render(FrameStateSharedPtr frameState);

    void bindTexture(int32_t index);

    void debugTexture(const std::string& widgetIdentityInfo);

protected:
    explicit Texture(ImageType imageType);

    bool deployTexture();

private:
    void startLoadImage();

    void startLoadBasis();

    void reUploadTexture(const char* result);

protected:
    TextureInfoSharedPtr m_textureInfo = std::make_shared<TextureInfo>();
    HwTexture2D m_textureHandle{0};
    TextureData m_textureData;

private:
    DebugObjectHandle m_debugObject{DebugObjectCategory::Texture, "Texture"};
    std::string m_uniqueID = Math::generate_uuid();
    uint64_t m_revision = 1;
};

using TextureSharedPtr = std::shared_ptr<Texture>;
using TextureWeakPtr = std::weak_ptr<Texture>;
}  // namespace morrow

#endif  // MORROW_TEXTURE_H
