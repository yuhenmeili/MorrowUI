//
// Created by lance on 24-7-1.
//

#ifndef MORROW_RENDERER_TEXTUREREGION_H_
#define MORROW_RENDERER_TEXTUREREGION_H_

#include "Texture.h"

namespace morrow
{
class TextureRegion
{
public:
    TextureRegion() = default;

    virtual ~TextureRegion() = default;

    explicit TextureRegion(TextureSharedPtr texture);

    // Constructor with a Texture pointer and specified width and height for the region
    TextureRegion(TextureSharedPtr texture, int32_t width, int32_t height);

    TextureRegion(TextureSharedPtr texture, int32_t x, int32_t y, int32_t width, int32_t height);

    TextureRegion(TextureSharedPtr texture, float u, float v, float u2, float v2);

    TextureRegion(TextureRegion& region);

    TextureRegion(TextureRegion region, int32_t x, int32_t y, int32_t width, int32_t height);

    void setRegion(TextureSharedPtr texture);

    void setRegion(int32_t x, int32_t y, int32_t width, int32_t height);

    void setRegion(float u, float v, float u2, float v2);

    void setRegion(TextureRegion& region);

    void setRegion(TextureRegion& region, int32_t x, int32_t y, int32_t width, int32_t height);

    void setTexture(TextureSharedPtr texture);

    TextureSharedPtr getTexture();

    float getU();

    void setU(float u);

    float getV();

    void setV(float v);

    float getU2();

    void setU2(float u2);

    float getV2() const;

    void setV2(float v2);

    int32_t getRegionX();

    void setRegionX(int32_t x);

    int32_t getRegionY();

    void setRegionY(int32_t y);

    int32_t getRegionWidth() const;

    void setRegionWidth(int32_t width);

    int32_t getRegionHeight() const;

    void setRegionHeight(int32_t height);

    virtual void flip(bool x, bool y);

    bool isFlipX() const;

    bool isFlipY() const;

    void scroll(float xAmount, float yAmount);

//    std::vector<std::vector<TextureRegionSharedPtr>> split(int32_t tileWidth, int32_t tileHeight);

//    static std::vector<std::vector<TextureRegionSharedPtr>> split(TextureSharedPtr texture, int32_t tileWidth, int32_t tileHeight);

private:
    TextureSharedPtr m_texture;
    float m_u;
    float m_v;
    float m_u2;
    float m_v2;
    int32_t m_regionWidth;
    int32_t m_regionHeight;
};

using TextureRegionSharedPtr = std::shared_ptr<TextureRegion>;
}

#endif //MORROW_RENDERER_TEXTUREREGION_H_
