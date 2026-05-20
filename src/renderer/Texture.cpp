//
// Created by lance on 2022/10/11.
//

#include <fstream>
#include <iostream>
#include "Texture.h"
#include "MathUtils.h"
#include "../utils/Log.h"
#include "PixelFormat.h"
#include "BasisTextureLoader.h"
#include "ToolUtils.h"
#include "GlobalObject.h"
#include "TextureLoader.h"
#include "TextureManager.h"

namespace morrow {
TextureSharedPtr Texture::create(ImageType imageType) {
    TextureSharedPtr texturePtr(new Texture(imageType));
    GlobalObject::getInstance().getTextureManager()->addTexture(texturePtr->m_uniqueID, texturePtr);
    return texturePtr;
}

Texture::Texture(ImageType imageType) {
    m_textureInfo->imageType = imageType;
}

Texture::~Texture() {
    LOG_I("cleanupGPU texture {}", getImageInfo().c_str());
    if (m_texture2DPtr) {
        RENDERINGTHREAD->deleteTexture2D(m_texture2DPtr);
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
        REQUESTRENDER;
    }
    return *this;
}

Texture& Texture::setTextureData(std::shared_ptr<unsigned char> textureData,
                                 int32_t imageWidth, int32_t imageHeight, PixelDataFormat format, int32_t bytes,
                                 bool compressedTexture) {
    m_textureInfo->textureDataSharedPtr = textureData;
    m_textureInfo->imageWidth = imageWidth;
    m_textureInfo->imageHeight = imageHeight;
    m_textureInfo->format = format;
    m_textureInfo->bytes = bytes;
    m_textureInfo->compressedTexture = compressedTexture;
    m_textureInfo->textureNeedUpLoad = true;
    REQUESTRENDER;
    return *this;
}

Texture& Texture::setTextureData(void* textureData,
                                 int32_t imageWidth, int32_t imageHeight, PixelDataFormat format, int32_t bytes,
                                 bool compressedTexture) {
    m_textureInfo->textureDataRawPtr = textureData;
    m_textureInfo->imageWidth = imageWidth;
    m_textureInfo->imageHeight = imageHeight;
    m_textureInfo->format = format;
    m_textureInfo->bytes = bytes;
    m_textureInfo->compressedTexture = compressedTexture;
    m_textureInfo->textureNeedUpLoad = true;
    REQUESTRENDER;
    return *this;
}

Texture& Texture::setTextureName(std::string textureName) {
    m_textureInfo->textureName = textureName;
    return *this;
}

Texture& Texture::setFormat(PixelDataFormat format) {
    m_textureInfo->format = format;
    REQUESTRENDER;
    return *this;
}

Texture& Texture::setMinFilterType(SamplerMinFilter minFilterType) {
    m_textureInfo->minFilterType = minFilterType;
    REQUESTRENDER;
    return *this;
}

Texture& Texture::setMagFilterType(SamplerMagFilter magFilterType) {
    m_textureInfo->magFilterType = magFilterType;
    REQUESTRENDER;
    return *this;
}

Texture& Texture::setWidth(int32_t width) {
    m_textureInfo->imageWidth = width;
    REQUESTRENDER;
    return *this;
}

Texture& Texture::setHeight(int32_t height) {
    m_textureInfo->imageHeight = height;
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
    if (!m_texture2DPtr) {
        m_texture2DPtr = RENDERINGTHREAD->createTexture2D(m_textureInfo->imageType);
    }
    if (m_textureInfo->textureNeedUpLoad) {
        if (m_textureInfo->imageType == ImageType::IMAGE) {
            if (!m_textureInfo->textureDataSharedPtr && !m_textureInfo->textureDataRawPtr) {
                std::string fileSuffix = ".basis";
                if (ToolUtils::endsWith(m_textureInfo->imageUrl, fileSuffix)) {
                    startLoadBasis();
                } else {
                    startLoadImage();
                }
            }
        }
        deployTexture();
        m_textureInfo->textureNeedUpLoad = false;
    }
}

void Texture::bindTexture(int32_t index) {
    RENDERINGTHREAD->useTexture2D(m_texture2DPtr, index);
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
    // 若持有 shared_ptr 所有权，多线程路径直接共享引用，无需 memcpy 全部像素。
    // textureDataSharedPtr 可为 nullptr（原始指针路径），此时退化到池复用拷贝。
    m_textureData.pixelOwner = m_textureInfo->textureDataSharedPtr;
    //TODO 由外部传入
    // m_textureData.releaseCallback = nullptr;
    RENDERINGTHREAD->updateTexture2D(m_texture2DPtr, m_textureData);
    return true;
}

void Texture::startLoadImage() {
    LOG_I("start loadImage {}", getImageInfo());
    int32_t comp;
    int32_t re_comp = PixelFormat::componentsLength(m_textureInfo->format);
    unsigned char* textureData = TextureFromFile::load(
        m_textureInfo->imageUrl.c_str(), &m_textureInfo->imageWidth, &m_textureInfo->imageHeight, &comp, re_comp);
    if (!textureData) {
        LOG_I("loadImageFile {} failed", m_textureInfo->imageUrl);
        m_textureInfo->textureDataSharedPtr.reset();
        return;
    }

    /// shared_ptr will take the ownership of the raw pointer
    /// free is not needed
    /// stbi_image_free(textureData);
    m_textureInfo->textureDataSharedPtr = std::shared_ptr<unsigned char>(textureData, [](unsigned char* ptr) {
        TextureFromFile::free(ptr);
    });

    m_textureInfo->bytes = m_textureInfo->imageWidth * m_textureInfo->imageHeight * re_comp;
    m_textureInfo->compressedTexture = false;
}

void Texture::startLoadBasis() {
    auto loader = std::make_shared<BasisTextureLoader>();
    loader->load(m_textureInfo->imageUrl, m_textureInfo->basisData, m_textureInfo->format);
    auto imageInfo = loader->getImageDesc(0);
    m_textureInfo->textureDataSharedPtr = std::shared_ptr<unsigned char>(m_textureInfo->basisData.data(), std::default_delete<unsigned char[]>());
    m_textureInfo->imageWidth = imageInfo.m_width;
    m_textureInfo->imageHeight = imageInfo.m_height;
    m_textureInfo->compressedTexture = true;
    m_textureInfo->bytes = m_textureInfo->basisData.size();
}

void Texture::reUploadTexture(const char* result) {
    m_textureInfo->textureNeedUpLoad = true;
}
}
