//
// Created by lance on 24-7-1.
//

#include "AtlasRegion.h"

namespace morrow
{
AtlasRegion::AtlasRegion()
{

}

AtlasRegion::AtlasRegion(TextureSharedPtr texture, int32_t x, int32_t y, int32_t width, int32_t height) : TextureRegion(texture, x, y, width, height)
{
    this->m_originalWidth = width;
    this->m_originalHeight = height;
    this->m_packedWidth = width;
    this->m_packedHeight = height;
}

AtlasRegion::AtlasRegion(AtlasRegion& region)
{
    this->setRegion(region);
    this->m_index = region.m_index;
    this->m_name = region.m_name;
    this->m_offsetX = region.m_offsetX;
    this->m_offsetY = region.m_offsetY;
    this->m_packedWidth = region.m_packedWidth;
    this->m_packedHeight = region.m_packedHeight;
    this->m_originalWidth = region.m_originalWidth;
    this->m_originalHeight = region.m_originalHeight;
    this->m_rotate = region.m_rotate;
    this->m_degrees = region.m_degrees;
    this->m_names = region.m_names;
    this->m_values = region.m_values;
}

AtlasRegion::AtlasRegion(TextureRegion& region)
{
    this->setRegion(region);
    this->m_packedWidth = region.getRegionWidth();
    this->m_packedHeight = region.getRegionHeight();
    this->m_originalWidth = this->m_packedWidth;
    this->m_originalHeight = this->m_packedHeight;
}

void AtlasRegion::flip(bool x, bool y)
{
    TextureRegion::flip(x, y);
    if (x) {
        this->m_offsetX = (float) this->m_originalWidth - this->m_offsetX - this->getRotatedPackedWidth();
    }

    if (y) {
        this->m_offsetY = (float) this->m_originalHeight - this->m_offsetY - this->getRotatedPackedHeight();
    }

}

float AtlasRegion::getRotatedPackedWidth() const
{
    return this->m_rotate ? (float) this->m_packedHeight : (float) this->m_packedWidth;
}

float AtlasRegion::getRotatedPackedHeight() const
{
    return this->m_rotate ? (float) this->m_packedWidth : (float) this->m_packedHeight;
}

std::vector<int32_t> AtlasRegion::findValue(std::string& name)
{
    std::vector<int32_t> result;
    if (!this->m_names.empty()) {
        for (int32_t indexName = 0; indexName < this->m_names.size(); ++indexName) {
            if (name == this->m_names[indexName]) {
                return this->m_values[indexName];
            }
        }
    }
    return result;
}

std::string AtlasRegion::toString()
{
    return this->m_name;
}
} // MORROWGUI