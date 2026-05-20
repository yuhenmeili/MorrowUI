//
// Created by lance on 24-7-1.
//

#include "TextureRegion.h"

namespace morrow
{
TextureRegion::TextureRegion(TextureSharedPtr texture)
{
    this->m_texture = texture;
    setRegion(0, 0, texture->getWidth(), texture->getHeight());
}

TextureRegion::TextureRegion(TextureSharedPtr texture, int32_t width, int32_t height)
{
    this->m_texture = texture;
    setRegion(0, 0, width, height);
}

TextureRegion::TextureRegion(TextureSharedPtr texture, int32_t x, int32_t y, int32_t width, int32_t height)
{
    this->m_texture = texture;
    this->setRegion(x, y, width, height);
}

TextureRegion::TextureRegion(TextureSharedPtr texture, float u, float v, float u2, float v2)
{
    this->m_texture = texture;
    this->setRegion(u, v, u2, v2);
}

TextureRegion::TextureRegion(TextureRegion& region)
{
    this->setRegion(region);
}

TextureRegion::TextureRegion(TextureRegion region, int32_t x, int32_t y, int32_t width, int32_t height)
{
    this->setRegion(region, x, y, width, height);
}

void TextureRegion::setRegion(TextureSharedPtr texture)
{
    this->m_texture = texture;
    this->setRegion(0, 0, texture->getWidth(), texture->getHeight());
}

void TextureRegion::setRegion(int32_t x, int32_t y, int32_t width, int32_t height)
{
    float invTexWidth = 1.0F / (float) this->m_texture->getWidth();
    float invTexHeight = 1.0F / (float) this->m_texture->getHeight();
    this->setRegion((float) x * invTexWidth, (float) y * invTexHeight, (float) (x + width) * invTexWidth, (float) (y + height) * invTexHeight);
    this->m_regionWidth = std::abs(width);
    this->m_regionHeight = std::abs(height);
}

void TextureRegion::setRegion(float u, float v, float u2, float v2)
{
    int32_t texWidth = m_texture->getWidth();
    int32_t texHeight = m_texture->getHeight();
    this->m_regionWidth = std::round(std::abs(u2 - u) * (float) texWidth);
    this->m_regionHeight = std::round(std::abs(v2 - v) * (float) texHeight);
    if (this->m_regionWidth == 1 && this->m_regionHeight == 1) {
        float adjustX = 0.25F / (float) texWidth;
        u += adjustX;
        u2 -= adjustX;
        float adjustY = 0.25F / (float) texHeight;
        v += adjustY;
        v2 -= adjustY;
    }
    this->m_u = u;
    this->m_v = v;
    this->m_u2 = u2;
    this->m_v2 = v2;
}

void TextureRegion::setRegion(TextureRegion& region)
{
    this->m_texture = region.m_texture;
    this->setRegion(region.m_u, region.m_v, region.m_u2, region.m_v2);
}

void TextureRegion::setRegion(TextureRegion& region, int32_t x, int32_t y, int32_t width, int32_t height)
{
    this->m_texture = region.m_texture;
    this->setRegion(region.getRegionX() + x, region.getRegionY() + y, width, height);
}

void TextureRegion::setTexture(TextureSharedPtr texture)
{
    this->m_texture = texture;
}

TextureSharedPtr TextureRegion::getTexture()
{
    return this->m_texture;
}

float TextureRegion::getU()
{
    return this->m_u;
}

void TextureRegion::setU(float u)
{
    this->m_u = u;
    this->m_regionWidth = std::round(std::abs(this->m_u2 - u) * (float) this->m_texture->getWidth());
}

float TextureRegion::getV()
{
    return this->m_v;
}

void TextureRegion::setV(float v)
{
    this->m_v = v;
    this->m_regionHeight = std::round(std::abs(this->m_v2 - v) * (float) this->m_texture->getHeight());
}

float TextureRegion::getU2()
{
    return this->m_u2;
}

void TextureRegion::setU2(float u2)
{
    this->m_u2 = u2;
    this->m_regionWidth = std::round(std::abs(u2 - this->m_u) * (float) this->m_texture->getWidth());
}

float TextureRegion::getV2() const
{
    return this->m_v2;
}

void TextureRegion::setV2(float v2)
{
    this->m_v2 = v2;
    this->m_regionHeight = std::round(std::abs(v2 - this->m_v) * (float) this->m_texture->getHeight());
}

int32_t TextureRegion::getRegionX()
{
    return std::round(this->m_u * (float) this->m_texture->getWidth());
}

void TextureRegion::setRegionX(int32_t x)
{
    this->setU((float) x / (float) this->m_texture->getWidth());
}

int32_t TextureRegion::getRegionY()
{
    return std::round(this->m_v * (float) this->m_texture->getHeight());
}

void TextureRegion::setRegionY(int32_t y)
{
    this->setV((float) y / (float) this->m_texture->getHeight());
}

int32_t TextureRegion::getRegionWidth() const
{
    return this->m_regionWidth;
}

void TextureRegion::setRegionWidth(int32_t width)
{
    if (this->isFlipX()) {
        this->setU(this->m_u2 + (float) width / (float) this->m_texture->getWidth());
    } else {
        this->setU2(this->m_u + (float) width / (float) this->m_texture->getWidth());
    }

}

int32_t TextureRegion::getRegionHeight() const
{
    return this->m_regionHeight;
}

void TextureRegion::setRegionHeight(int32_t height)
{
    if (this->isFlipY()) {
        this->setV(this->m_v2 + (float) height / (float) this->m_texture->getHeight());
    } else {
        this->setV2(this->m_v + (float) height / (float) this->m_texture->getHeight());
    }

}

void TextureRegion::flip(bool x, bool y)
{
    float temp;
    if (x) {
        temp = this->m_u;
        this->m_u = this->m_u2;
        this->m_u2 = temp;
    }

    if (y) {
        temp = this->m_v;
        this->m_v = this->m_v2;
        this->m_v2 = temp;
    }

}

bool TextureRegion::isFlipX() const
{
    return this->m_u > this->m_u2;
}

bool TextureRegion::isFlipY() const
{
    return this->m_v > this->m_v2;
}

void TextureRegion::scroll(float xAmount, float yAmount)
{
    float height;
    if (xAmount != 0.0F) {
        height = (this->m_u2 - this->m_u) * (float) this->m_texture->getWidth();
        this->m_u = fmodf(this->m_u + xAmount, 1.0f);
        this->m_u2 = this->m_u + height / (float) this->m_texture->getWidth();
    }

    if (yAmount != 0.0F) {
        height = (this->m_v2 - this->m_v) * (float) this->m_texture->getHeight();
        this->m_v = fmodf(this->m_v + yAmount, 1.0f);
        this->m_v2 = this->m_v + height / (float) this->m_texture->getHeight();
    }

}

//std::vector<std::vector<TextureRegionSharedPtr>> TextureRegion::split(int32_t tileWidth, int32_t tileHeight)
//{
//    int32_t x = this->getRegionX();
//    int32_t y = this->getRegionY();
//    int32_t width = this->m_regionWidth;
//    int32_t height = this->m_regionHeight;
//    int32_t rows = height / tileHeight;
//    int32_t cols = width / tileWidth;
//    int32_t startX = x;
//    std::vector<std::vector<TextureRegionSharedPtr>> tiles(rows, std::vector<TextureRegionSharedPtr>(cols));
//
//    for (int32_t row = 0; row < rows; y += tileHeight) {
//        x = startX;
//
//        for (int32_t col = 0; col < cols; x += tileWidth) {
//            tiles[row][col] = std::make_shared<TextureRegion>(this->m_texture, x, y, tileWidth, tileHeight);
//            ++col;
//        }
//
//        ++row;
//    }
//    return tiles;
//}
//
//std::vector<std::vector<TextureRegionSharedPtr>> TextureRegion::split(TextureSharedPtr texture, int32_t tileWidth, int32_t tileHeight)
//{
//    TextureRegionSharedPtr region = std::make_shared<TextureRegion>(texture);
//    return region->split(tileWidth, tileHeight);
//}

};
