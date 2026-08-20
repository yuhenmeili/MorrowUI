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

/// 支持普通纹理、图集区域、圆角和裁剪的图片组件。
class MRImage : public UIWidget {
public:
    /// 创建一个图片组件。
    static MRImageSharedPtr create();

    /// 销毁图片组件。
    virtual ~MRImage() = default;

    /// 设置图片圆角大小。
    void setRounding(float rounding);

    /// 设置图片裁剪区域。
    void setScissor(float x, float y, float width, float height);

    /// 设置图片使用的普通纹理。
    void setTexture(TextureSharedPtr texture);

    /// 设置图片使用的纹理图集及其中的区域名称。
    void setTexture(TextureAtlasSharedPtr texture, const std::string& textureName);

    /// 输出当前图片纹理的调试信息。
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
