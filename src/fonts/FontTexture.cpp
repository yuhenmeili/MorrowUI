//
// Created by 0060328 on 25-10-9.
//

#include "FontTexture.h"

#include <cstring>

#include "GlobalObject.h"

namespace morrow {
FontTexture::FontTexture(int32_t width, int32_t height): Texture(ImageType::TEXT) {
    m_fontData.resize(static_cast<size_t>(width) * height * 4, 0);
    setTextureData(m_fontData.data(), width, height, PixelDataFormat::RGBA);
    // 图集存 SDF（有向距离场），重建依赖双线性插值，必须使用 LINEAR 过滤
    setMinFilterType(SamplerMinFilter::LINEAR);
    setMagFilterType(SamplerMagFilter::LINEAR);
}

bool FontTexture::Initialize() {
    if (!m_textureHandle) {
        m_textureHandle = RENDERINGTHREAD->createTexture2D(m_textureInfo->imageType);
    }
    if (m_textureInfo->textureNeedUpLoad) {
        deployTexture();
        m_textureInfo->textureNeedUpLoad = false;
    }
    return true;
}

void FontTexture::UpdateRegion(int32_t x, int32_t y, int32_t width, int32_t height, const unsigned char* sourceData) {
    const int32_t imageWidth = getWidth();
    for (int row = 0; row < height; ++row) {
        unsigned char* dest = m_fontData.data() + (static_cast<size_t>(y + row) * imageWidth + x) * 4;
        const unsigned char* src = sourceData + row * width;
        for (int col = 0; col < width; ++col) {
            const unsigned char value = src[col];
            dest[col * 4 + 0] = value;
            dest[col * 4 + 1] = value;
            dest[col * 4 + 2] = value;
            dest[col * 4 + 3] = value;
        }
    }
    m_dirty = true;
}

void FontTexture::FlushToGPU() {
    if (!m_dirty || !m_textureHandle) return;
    RENDERINGTHREAD->updateSubTexture2D(m_textureHandle, m_textureData, 0, 0, getWidth(), getHeight(), m_fontData.data());
    m_dirty = false;
}
} // morrow
