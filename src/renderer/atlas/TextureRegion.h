//
// Created by lance on 24-7-1.
//

#ifndef MORROW_RENDERER_TEXTUREREGION_H_
#define MORROW_RENDERER_TEXTUREREGION_H_

#include <string>
#include <vector>
#include "AtlasParser.h"
#include "Texture.h"

namespace morrow {
/// 纹理子区域：持有纹理与 UV 矩形；由图集帧构造时附带图集元数据。
///
/// 非图集场景下元数据（name/index/offset 等）保持默认值，可当作纯 UV 矩形使用。
class TextureRegion {
public:
    TextureRegion() = default;

    ~TextureRegion() = default;

    explicit TextureRegion(TextureSharedPtr texture);

    TextureRegion(TextureSharedPtr texture, int32_t width, int32_t height);

    TextureRegion(TextureSharedPtr texture, int32_t x, int32_t y, int32_t width, int32_t height);

    TextureRegion(TextureSharedPtr texture, float u, float v, float u2, float v2);

    explicit TextureRegion(const TextureRegion& region);

    /// 由图集帧构造：换算 UV、填充图集元数据，并按帧标记应用翻转。
    TextureRegion(const AtlasFrameSharedPtr& frame, const TextureSharedPtr& texture);

    void setRegion(TextureSharedPtr texture);

    void setRegion(int32_t x, int32_t y, int32_t width, int32_t height);

    void setRegion(float u, float v, float u2, float v2);

    void setRegion(const TextureRegion& region);

    void setTexture(TextureSharedPtr texture);

    TextureSharedPtr getTexture() const;

    float getU() const;

    void setU(float u);

    float getV() const;

    void setV(float v);

    float getU2() const;

    void setU2(float u2);

    float getV2() const;

    void setV2(float v2);

    int32_t getRegionX() const;

    void setRegionX(int32_t x);

    int32_t getRegionY() const;

    void setRegionY(int32_t y);

    int32_t getRegionWidth() const;

    void setRegionWidth(int32_t width);

    int32_t getRegionHeight() const;

    void setRegionHeight(int32_t height);

    /// 翻转 UV；图集区域同时调整偏移，保证锚点与翻转后的像素一致。
    void flip(bool x, bool y);

    bool isFlipX() const;

    bool isFlipY() const;

    void scroll(float xAmount, float yAmount);

    /// 图集区域元数据：区域名称，非图集区域为空。
    const std::string& getName() const {
        return m_name;
    }

    /// 图集区域元数据：帧序号，未编组为 -1。
    int32_t getIndex() const {
        return m_index;
    }

    float getOffsetX() const {
        return m_offsetX;
    }

    float getOffsetY() const {
        return m_offsetY;
    }

    /// 打包进图集后的宽高（已考虑旋转）。
    int32_t getPackedWidth() const {
        return m_packedWidth;
    }

    int32_t getPackedHeight() const {
        return m_packedHeight;
    }

    /// 原始（未裁剪、未旋转）宽高。
    int32_t getOriginalWidth() const {
        return m_originalWidth;
    }

    int32_t getOriginalHeight() const {
        return m_originalHeight;
    }

    bool isRotated() const {
        return m_rotate;
    }

    int32_t getDegrees() const {
        return m_degrees;
    }

    /// 打包宽度按旋转修正后的值。
    float getRotatedPackedWidth() const;

    /// 打包高度按旋转修正后的值。
    float getRotatedPackedHeight() const;

    /// 查询打包时附加的整数值（libGDX 扩展字段）。
    std::vector<int32_t> findValue(const std::string& name) const;

private:
    TextureSharedPtr m_texture;
    float m_u = 0.0f;
    float m_v = 0.0f;
    float m_u2 = 0.0f;
    float m_v2 = 0.0f;
    int32_t m_regionWidth = 0;
    int32_t m_regionHeight = 0;

    std::string m_name;
    int32_t m_index = -1;
    float m_offsetX = 0.0f;
    float m_offsetY = 0.0f;
    int32_t m_packedWidth = 0;
    int32_t m_packedHeight = 0;
    int32_t m_originalWidth = 0;
    int32_t m_originalHeight = 0;
    bool m_rotate = false;
    int32_t m_degrees = 0;
    std::vector<std::string> m_names;
    std::vector<std::vector<int32_t>> m_values;
};

using TextureRegionSharedPtr = std::shared_ptr<TextureRegion>;
}

#endif //MORROW_RENDERER_TEXTUREREGION_H_