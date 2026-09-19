//
// Created by lance on 2022/10/11.
//

#include "Texture.h"

#include <fstream>
#include <iostream>

#include "GlobalObject.h"
#include "MathUtils.h"
#include "PixelFormat.h"
#include "TextureLoader.h"
#include "TextureManager.h"
#include "ToolUtils.h"
#include "utils/Log.h"
#if MORROW_ENABLE_BASISU
#include "BasisTextureLoader.h"
#include "Ktx2TextureLoader.h"
#endif

namespace {
/// 压缩像素格式决定上传走 glCompressedTexImage2D 还是 glTexImage2D。
bool isCompressedPixelFormat(PixelDataFormat format) {
    return format == PixelDataFormat::COMPRESSED_RGB8_ETC2 || format == PixelDataFormat::COMPRESSED_RGBA8_ETC2_EAC;
}
}

namespace morrow {
TextureSharedPtr Texture::create(ImageType imageType) {
    TextureSharedPtr texturePtr(new Texture(imageType));
    GlobalObject::getInstance().getTextureManager()->addTexture(texturePtr->m_uniqueID, texturePtr);
    return texturePtr;
}

Texture::Texture(ImageType imageType) {
    m_textureInfo->imageType = imageType;
    m_debugObject.setName(m_uniqueID);
}

Texture::~Texture() {
    LOG_I("cleanupGPU texture {}", getImageInfo().c_str());
    if (m_textureHandle.isValid()) {
        RENDERINGTHREAD->deleteTexture2D(m_textureHandle);
    }
    m_textureInfo->textureDataRawPtr = nullptr;
    GlobalObject::getInstance().getTextureManager()->removeTexture(m_uniqueID);
}

Texture& Texture::setImageUrl(const std::string& imageUrl) {
    if (m_textureInfo->imageUrl != imageUrl) {
        LOG_I("setImageUrl {}", imageUrl);
        m_textureInfo->imageUrl = imageUrl;
        m_textureInfo->textureDataSharedPtr.reset();
        m_textureInfo->textureDataRawPtr = nullptr;
        m_textureInfo->textureNeedUpLoad = true;
        ++m_revision;
        REQUESTRENDER;
    }
    return *this;
}

Texture& Texture::setTextureData(std::shared_ptr<unsigned char> textureData, int32_t imageWidth, int32_t imageHeight, PixelDataFormat format, int32_t bytes,
                                 bool compressedTexture) {
    m_textureInfo->textureDataSharedPtr = textureData;
    m_textureInfo->textureDataBuffer.reset();
    m_textureInfo->textureDataRawPtr = nullptr;
    m_textureInfo->gpuUseCompleteCallback = {};
    m_textureInfo->imageWidth = imageWidth;
    m_textureInfo->imageHeight = imageHeight;
    m_textureInfo->format = format;
    m_textureInfo->bytes = bytes;
    m_textureInfo->compressedTexture = compressedTexture;
    m_textureInfo->textureNeedUpLoad = true;
    ++m_revision;
    REQUESTRENDER;
    return *this;
}

Texture& Texture::setTextureData(std::shared_ptr<std::vector<unsigned char>> textureData, int32_t imageWidth, int32_t imageHeight, PixelDataFormat format, bool compressedTexture) {
    m_textureInfo->textureDataSharedPtr.reset();
    m_textureInfo->textureDataBuffer = std::move(textureData);
    m_textureInfo->textureDataRawPtr = m_textureInfo->textureDataBuffer ? m_textureInfo->textureDataBuffer->data() : nullptr;
    m_textureInfo->imageWidth = imageWidth;
    m_textureInfo->imageHeight = imageHeight;
    m_textureInfo->format = format;
    m_textureInfo->bytes = m_textureInfo->textureDataBuffer ? static_cast<int32_t>(m_textureInfo->textureDataBuffer->size()) : 0;
    m_textureInfo->compressedTexture = compressedTexture;
    m_textureInfo->imageType = ImageType::IMAGE;
    m_textureInfo->gpuUseCompleteCallback = {};
    m_textureInfo->textureNeedUpLoad = true;
    ++m_revision;
    REQUESTRENDER;
    return *this;
}

Texture& Texture::setTextureData(void* textureData, int32_t imageWidth, int32_t imageHeight, PixelDataFormat format, int32_t bytes, bool compressedTexture) {
    m_textureInfo->textureDataSharedPtr.reset();
    m_textureInfo->textureDataBuffer.reset();
    m_textureInfo->textureDataRawPtr = textureData;
    m_textureInfo->gpuUseCompleteCallback = {};
    m_textureInfo->imageWidth = imageWidth;
    m_textureInfo->imageHeight = imageHeight;
    m_textureInfo->format = format;
    m_textureInfo->bytes = bytes;
    m_textureInfo->compressedTexture = compressedTexture;
    m_textureInfo->textureNeedUpLoad = true;
    ++m_revision;
    REQUESTRENDER;
    return *this;
}

Texture& Texture::setOESTextureData(void* textureData, int32_t imageWidth, int32_t imageHeight, PixelDataFormat format, int32_t bytes,
                                    std::function<void()> gpuUseCompleteCallback) {
    m_textureInfo->textureDataSharedPtr.reset();
    m_textureInfo->textureDataRawPtr = textureData;
    m_textureInfo->imageWidth = imageWidth;
    m_textureInfo->imageHeight = imageHeight;
    m_textureInfo->format = format;
    m_textureInfo->bytes = bytes;
    m_textureInfo->compressedTexture = false;
    m_textureInfo->imageType = ImageType::OES;
    m_textureInfo->gpuUseCompleteCallback = std::move(gpuUseCompleteCallback);
    m_textureInfo->textureNeedUpLoad = true;
    ++m_revision;
    REQUESTRENDER;
    return *this;
}

Texture& Texture::setTextureName(std::string textureName) {
    m_textureInfo->textureName = textureName;
    m_debugObject.setName(m_textureInfo->textureName);
    return *this;
}

Texture& Texture::setFormat(PixelDataFormat format) {
    m_textureInfo->format = format;
    ++m_revision;
    REQUESTRENDER;
    return *this;
}

Texture& Texture::setMinFilterType(SamplerMinFilter minFilterType) {
    m_textureInfo->minFilterType = minFilterType;
    ++m_revision;
    REQUESTRENDER;
    return *this;
}

Texture& Texture::setMagFilterType(SamplerMagFilter magFilterType) {
    m_textureInfo->magFilterType = magFilterType;
    ++m_revision;
    REQUESTRENDER;
    return *this;
}

Texture& Texture::setWidth(int32_t width) {
    m_textureInfo->imageWidth = width;
    ++m_revision;
    REQUESTRENDER;
    return *this;
}

Texture& Texture::setHeight(int32_t height) {
    m_textureInfo->imageHeight = height;
    ++m_revision;
    REQUESTRENDER;
    return *this;
}

ImageType Texture::getImageType() const {
    return m_textureInfo->imageType;
}

int32_t Texture::getWidth() {
    return m_textureInfo->imageWidth;
}

int32_t Texture::getHeight() {
    return m_textureInfo->imageHeight;
}

uint64_t Texture::getRevision() const {
    return m_revision;
}

std::string Texture::getImageInfo() {
    auto url = m_textureInfo->imageUrl;
    if (!m_textureInfo->textureName.empty()) {
        url += "_" + m_textureInfo->textureName;
    }
    return url;
}

void Texture::prepare() {
    render(nullptr);
}

void Texture::render(FrameStateSharedPtr frameState) {
    if (!m_textureHandle.isValid()) {
        m_textureHandle = RENDERINGTHREAD->createTexture2D(m_textureInfo->imageType);
    }
        if (m_textureInfo->textureNeedUpLoad) {
            if (m_textureInfo->imageType == ImageType::IMAGE) {
                if (!m_textureInfo->textureDataSharedPtr && !m_textureInfo->textureDataRawPtr) {
#if MORROW_ENABLE_BASISU
                    if (ToolUtils::endsWith(m_textureInfo->imageUrl, ".basis")) {
                        startLoadBasis();
                    } else if (ToolUtils::endsWith(m_textureInfo->imageUrl, ".ktx2")) {
                        startLoadKtx2();
                    } else {
                        startLoadImage();
                    }
#else
                    if (ToolUtils::endsWith(m_textureInfo->imageUrl, ".basis") ||
                        ToolUtils::endsWith(m_textureInfo->imageUrl, ".ktx2")) {
                        LOG_E("texture {} requires MORROW_ENABLE_BASISU=ON", m_textureInfo->imageUrl);
                        m_textureInfo->textureNeedUpLoad = false;
                        return;
                    }
                    startLoadImage();
#endif
                }
            }
            deployTexture();
            m_textureInfo->textureNeedUpLoad = false;
        }
}

void Texture::bindTexture(int32_t index) {
    RENDERINGTHREAD->useTexture2D(m_textureHandle, index);
}

void Texture::debugTexture(const std::string& widgetIdentityInfo) {
    if (m_textureInfo->textureDataSharedPtr || m_textureInfo->textureDataRawPtr) {
        void* dataPtr = m_textureInfo->textureDataSharedPtr ? m_textureInfo->textureDataSharedPtr.get() : m_textureInfo->textureDataRawPtr;
        std::string imageInfo = widgetIdentityInfo + "_" + getImageInfo();
        std::replace(imageInfo.begin(), imageInfo.end(), '/', '_');
        std::string path = "image_" + imageInfo + "_" + Math::generate_uuid();
        ToolUtils::debugTexture(dataPtr, m_textureInfo->imageWidth, m_textureInfo->imageHeight, path, m_textureInfo->imageType, m_textureInfo->format);
    } else {
        LOG_I("{}", (widgetIdentityInfo + "_" + getImageInfo() + "no textureData to save"));
    }
}

bool Texture::deployTexture() {
    if (!m_textureInfo->textureDataSharedPtr && !m_textureInfo->textureDataRawPtr) {
        return false;
    }
    auto dataPtr = m_textureInfo->textureDataSharedPtr ? m_textureInfo->textureDataSharedPtr.get() : m_textureInfo->textureDataRawPtr;
    m_textureData.pixels = dataPtr;
    m_textureData.width = m_textureInfo->imageWidth;
    m_textureData.height = m_textureInfo->imageHeight;
    m_textureData.format = m_textureInfo->format;
    m_textureData.bytes = m_textureInfo->bytes;
    m_textureData.compressedTexture = m_textureInfo->compressedTexture;
    m_textureData.minFilterType = m_textureInfo->minFilterType;
    m_textureData.magFilterType = m_textureInfo->magFilterType;
    m_textureData.imageType = m_textureInfo->imageType;
    m_textureData.cpuPixelOwner =
        m_textureInfo->textureDataBuffer ? std::static_pointer_cast<void>(m_textureInfo->textureDataBuffer) : std::static_pointer_cast<void>(m_textureInfo->textureDataSharedPtr);
    m_textureData.gpuUseCompleteCallback = std::move(m_textureInfo->gpuUseCompleteCallback);
    RENDERINGTHREAD->updateTexture2D(m_textureHandle, m_textureData);

    // updateTexture2D copies the TextureData into the command payload (or
    // consumes it immediately in single-threaded mode). The Texture object
    // therefore need not retain the CPU upload buffer after submission.
    m_textureInfo->textureDataSharedPtr.reset();
    m_textureInfo->textureDataBuffer.reset();
    m_textureInfo->textureDataRawPtr = nullptr;
    m_textureData.pixels = nullptr;
    m_textureData.cpuPixelOwner.reset();
    return true;
}

void Texture::startLoadImage() {
    LOG_I("start loadImage {}", getImageInfo());
    int32_t comp;
    int32_t re_comp = PixelFormat::componentsLength(m_textureInfo->format);
    unsigned char* textureData = TextureFromFile::load(m_textureInfo->imageUrl.c_str(), &m_textureInfo->imageWidth, &m_textureInfo->imageHeight, &comp, re_comp);
    if (!textureData) {
        LOG_I("loadImageFile {} failed", m_textureInfo->imageUrl);
        m_textureInfo->textureDataSharedPtr.reset();
        return;
    }

    /// shared_ptr will take the ownership of the raw pointer
    /// free is not needed
    /// stbi_image_free(textureData);
    m_textureInfo->textureDataSharedPtr = std::shared_ptr<unsigned char>(textureData, [](unsigned char* ptr) { TextureFromFile::free(ptr); });

    m_textureInfo->bytes = m_textureInfo->imageWidth * m_textureInfo->imageHeight * re_comp;
    m_textureInfo->compressedTexture = false;
    m_onLoaded.notify();
}

void Texture::startLoadBasis() {
#if MORROW_ENABLE_BASISU
    auto loader = std::make_shared<BasisTextureLoader>();
    loader->load(m_textureInfo->imageUrl, m_textureInfo->basisData, m_textureInfo->format);
    auto imageInfo = loader->getImageDesc(0);
    // basisData（basisu::vector）保持像素所有权；textureDataSharedPtr 仅作
    // 别名视图：deleter 持有 TextureInfo 引用保证像素存活到 GPU 上传命令
    // 执行完毕，最后一个引用释放时归还 basisData。此前用 delete[] 包装
    // vector 内部指针，会在 deployTexture 的 reset() 与 vector 析构时双重释放。
    m_textureInfo->textureDataSharedPtr = std::shared_ptr<unsigned char>(
        m_textureInfo->basisData.data(),
        [owner = m_textureInfo](unsigned char*) { owner->basisData.clear(); });
    m_textureInfo->imageWidth = imageInfo.m_width;
    m_textureInfo->imageHeight = imageInfo.m_height;
    m_textureInfo->compressedTexture = isCompressedPixelFormat(m_textureInfo->format);
    m_textureInfo->bytes = m_textureInfo->basisData.size();
    m_onLoaded.notify();
#endif
}

void Texture::startLoadKtx2() {
#if MORROW_ENABLE_BASISU
    auto loader = std::make_shared<Ktx2TextureLoader>();
    if (!loader->load(m_textureInfo->imageUrl, m_textureInfo->basisData, m_textureInfo->format)) {
        LOG_I("loadKtx2File {} failed", m_textureInfo->imageUrl);
        m_textureInfo->basisData.clear();
        return;
    }
    // 所有权语义同 startLoadBasis：basisData 持有，shared_ptr 仅作生命周期别名。
    m_textureInfo->textureDataSharedPtr = std::shared_ptr<unsigned char>(
        m_textureInfo->basisData.data(),
        [owner = m_textureInfo](unsigned char*) { owner->basisData.clear(); });
    m_textureInfo->imageWidth = loader->getWidth();
    m_textureInfo->imageHeight = loader->getHeight();
    m_textureInfo->compressedTexture = loader->isCompressed();
    m_textureInfo->bytes = m_textureInfo->basisData.size();
    m_onLoaded.notify();
#endif
}

void Texture::reUploadTexture(const char* result) {
    m_textureInfo->textureNeedUpLoad = true;
}
}  // namespace morrow
