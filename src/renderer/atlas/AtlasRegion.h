//
// Created by lance on 24-7-1.
//

#ifndef MORROW_RENDERER_ATLASREGION_H_
#define MORROW_RENDERER_ATLASREGION_H_

#include <string>
#include "TextureRegion.h"

namespace morrow
{
class AtlasRegion : public TextureRegion
{
public:
    AtlasRegion();

    AtlasRegion(TextureSharedPtr texture, int32_t x, int32_t y, int32_t width, int32_t height);

    AtlasRegion(AtlasRegion& region);

    explicit AtlasRegion(TextureRegion& region);

    void flip(bool x, bool y) override;

    float getRotatedPackedWidth() const;

    float getRotatedPackedHeight() const;

    std::vector<int32_t> findValue(std::string& name);

    std::string toString();

public:
    int32_t m_index = -1;
    std::string m_name;
    float m_offsetX;
    float m_offsetY;
    int32_t m_packedWidth;
    int32_t m_packedHeight;
    int32_t m_originalWidth;
    int32_t m_originalHeight;
    bool m_rotate;
    int32_t m_degrees;
    std::vector<std::string> m_names;
    std::vector<std::vector<int32_t>> m_values;
};

using AtlasRegionSharedPtr = std::shared_ptr<AtlasRegion>;
} // MORROWGUI
#endif //MORROW_RENDERER_ATLASREGION_H_
