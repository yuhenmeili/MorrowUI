//
// Created by lance on 24-7-1.
//

#include <algorithm>
#include <cmath>
#include "TextureRegion.h"

namespace morrow {
TextureRegion::TextureRegion(TextureSharedPtr texture) {
    m_texture = std::move(texture);
    setRegion(0, 0, m_texture->getWidth(), m_texture->getHeight());
}

TextureRegion::TextureRegion(TextureSharedPtr texture, int32_t width, int32_t height) {
    m_texture = std::move(texture);
    setRegion(0, 0, width, height);
}

TextureRegion::TextureRegion(TextureSharedPtr texture, int32_t x, int32_t y, int32_t width, int32_t height) {
    m_texture = std::move(texture);
    setRegion(x, y, width, height);
}

TextureRegion::TextureRegion(TextureSharedPtr texture, float u, float v, float u2, float v2) {
    m_texture = std::move(texture);
    setRegion(u, v, u2, v2);
}

TextureRegion::TextureRegion(const TextureRegion& region) {
    setRegion(region);
    m_name = region.m_name;
    m_index = region.m_index;
    m_offsetX = region.m_offsetX;
    m_offsetY = region.m_offsetY;
    m_packedWidth = region.m_packedWidth;
    m_packedHeight = region.m_packedHeight;
    m_originalWidth = region.m_originalWidth;
    m_originalHeight = region.m_originalHeight;
    m_rotate = region.m_rotate;
    m_degrees = region.m_degrees;
    m_names = region.m_names;
    m_values = region.m_values;
}

TextureRegion::TextureRegion(const AtlasFrameSharedPtr& frame, const TextureSharedPtr& texture) {
    m_texture = texture;
    // 旋转打包的区域，宽高在图集中已互换。
    setRegion(frame->left, frame->top, frame->rotate ? frame->height : frame->width, frame->rotate ? frame->width : frame->height);
    m_name = frame->name;
    m_index = frame->index;
    m_offsetX = frame->offsetX;
    m_offsetY = frame->offsetY;
    m_packedWidth = frame->rotate ? frame->height : frame->width;
    m_packedHeight = frame->rotate ? frame->width : frame->height;
    m_originalWidth = frame->originalWidth;
    m_originalHeight = frame->originalHeight;
    m_rotate = frame->rotate;
    m_degrees = frame->degrees;
    m_names = frame->names;
    m_values = frame->values;
    if (frame->flip) {
        flip(false, true);
    }
}

void TextureRegion::setRegion(TextureSharedPtr texture) {
    m_texture = std::move(texture);
    setRegion(0, 0, m_texture->getWidth(), m_texture->getHeight());
}

void TextureRegion::setRegion(int32_t x, int32_t y, int32_t width, int32_t height) {
    float invTexWidth = 1.0F / (float)m_texture->getWidth();
    float invTexHeight = 1.0F / (float)m_texture->getHeight();
    setRegion((float)x * invTexWidth, (float)y * invTexHeight, (float)(x + width) * invTexWidth, (float)(y + height) * invTexHeight);
    m_regionWidth = std::abs(width);
    m_regionHeight = std::abs(height);
}

void TextureRegion::setRegion(float u, float v, float u2, float v2) {
    int32_t texWidth = m_texture->getWidth();
    int32_t texHeight = m_texture->getHeight();
    m_regionWidth = std::round(std::abs(u2 - u) * (float)texWidth);
    m_regionHeight = std::round(std::abs(v2 - v) * (float)texHeight);
    if (m_regionWidth == 1 && m_regionHeight == 1) {
        float adjustX = 0.25F / (float)texWidth;
        u += adjustX;
        u2 -= adjustX;
        float adjustY = 0.25F / (float)texHeight;
        v += adjustY;
        v2 -= adjustY;
    }
    m_u = u;
    m_v = v;
    m_u2 = u2;
    m_v2 = v2;
}

void TextureRegion::setRegion(const TextureRegion& region) {
    m_texture = region.m_texture;
    setRegion(region.m_u, region.m_v, region.m_u2, region.m_v2);
}

void TextureRegion::setTexture(TextureSharedPtr texture) {
    m_texture = std::move(texture);
}

TextureSharedPtr TextureRegion::getTexture() const {
    return m_texture;
}

float TextureRegion::getU() const {
    return m_u;
}

void TextureRegion::setU(float u) {
    m_u = u;
    m_regionWidth = std::round(std::abs(m_u2 - u) * (float)m_texture->getWidth());
}

float TextureRegion::getV() const {
    return m_v;
}

void TextureRegion::setV(float v) {
    m_v = v;
    m_regionHeight = std::round(std::abs(m_v2 - v) * (float)m_texture->getHeight());
}

float TextureRegion::getU2() const {
    return m_u2;
}

void TextureRegion::setU2(float u2) {
    m_u2 = u2;
    m_regionWidth = std::round(std::abs(u2 - m_u) * (float)m_texture->getWidth());
}

float TextureRegion::getV2() const {
    return m_v2;
}

void TextureRegion::setV2(float v2) {
    m_v2 = v2;
    m_regionHeight = std::round(std::abs(v2 - m_v) * (float)m_texture->getHeight());
}

int32_t TextureRegion::getRegionX() const {
    return std::round(m_u * (float)m_texture->getWidth());
}

void TextureRegion::setRegionX(int32_t x) {
    setU((float)x / (float)m_texture->getWidth());
}

int32_t TextureRegion::getRegionY() const {
    return std::round(m_v * (float)m_texture->getHeight());
}

void TextureRegion::setRegionY(int32_t y) {
    setV((float)y / (float)m_texture->getHeight());
}

int32_t TextureRegion::getRegionWidth() const {
    return m_regionWidth;
}

void TextureRegion::setRegionWidth(int32_t width) {
    if (isFlipX()) {
        setU(m_u2 + (float)width / (float)m_texture->getWidth());
    } else {
        setU2(m_u + (float)width / (float)m_texture->getWidth());
    }
}

int32_t TextureRegion::getRegionHeight() const {
    return m_regionHeight;
}

void TextureRegion::setRegionHeight(int32_t height) {
    if (isFlipY()) {
        setV(m_v2 + (float)height / (float)m_texture->getHeight());
    } else {
        setV2(m_v + (float)height / (float)m_texture->getHeight());
    }
}

void TextureRegion::flip(bool x, bool y) {
    if (x) {
        std::swap(m_u, m_u2);
        m_offsetX = (float)m_originalWidth - m_offsetX - getRotatedPackedWidth();
    }

    if (y) {
        std::swap(m_v, m_v2);
        m_offsetY = (float)m_originalHeight - m_offsetY - getRotatedPackedHeight();
    }
}

bool TextureRegion::isFlipX() const {
    return m_u > m_u2;
}

bool TextureRegion::isFlipY() const {
    return m_v > m_v2;
}

void TextureRegion::scroll(float xAmount, float yAmount) {
    float size;
    if (xAmount != 0.0F) {
        size = (m_u2 - m_u) * (float)m_texture->getWidth();
        m_u = fmodf(m_u + xAmount, 1.0f);
        m_u2 = m_u + size / (float)m_texture->getWidth();
    }

    if (yAmount != 0.0F) {
        size = (m_v2 - m_v) * (float)m_texture->getHeight();
        m_v = fmodf(m_v + yAmount, 1.0f);
        m_v2 = m_v + size / (float)m_texture->getHeight();
    }
}

float TextureRegion::getRotatedPackedWidth() const {
    return m_rotate ? (float)m_packedHeight : (float)m_packedWidth;
}

float TextureRegion::getRotatedPackedHeight() const {
    return m_rotate ? (float)m_packedWidth : (float)m_packedHeight;
}

std::vector<int32_t> TextureRegion::findValue(const std::string& name) const {
    for (size_t i = 0; i < m_names.size(); ++i) {
        if (name == m_names[i]) {
            return m_values[i];
        }
    }
    return {};
}
} // MORROWGUI