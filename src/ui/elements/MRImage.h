//
// Created by lance on 2023/5/11.
//

#ifndef MORROW_IMAGE3D_H
#define MORROW_IMAGE3D_H

#include <string>
#include "Texture.h"
#include "atlas/TextureAtlas.h"
#include "base/UIWidget.h"

namespace morrow {
class MRImage;
using MRImageSharedPtr = std::shared_ptr<MRImage>;

class MRImage : public UIWidget {
public:
    static MRImageSharedPtr create();

    virtual ~MRImage() = default;

    void setRounding(float rounding);

    void setScissor(float x, float y, float width, float height);

    void setTexture(TextureSharedPtr texture);

    void setTexture(TextureAtlasSharedPtr texture, const std::string& textureName);

    void debugTexture() override;

private:
    MRImage();

    TextureSharedPtr m_texture;
    TextureAtlasSharedPtr m_textureAtlas;
    AtlasRegionSharedPtr m_atlasRegion;
    std::string m_atlasRegionName;

    float m_rounding = 0.0f;
};
}
#endif //MORROW_IMAGE3D_H
