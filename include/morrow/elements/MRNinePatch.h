//
// 九宫格图片：单张纹理按 inset 切为 3x3，四角固定、边缘拉伸、中间平铺拉伸。
// 实现为 9 个 MRImage 象限的复合控件，不改动渲染层。
//

#ifndef MORROW_MR_NINE_PATCH_H
#define MORROW_MR_NINE_PATCH_H

#include <array>
#include <memory>

#include "morrow/Texture.h"
#include "morrow/elements/MRImage.h"
#include "morrow/base/UIWidget.h"

namespace morrow {

class MRNinePatch : public UIWidget {
public:
    MRNinePatch();

    static std::shared_ptr<MRNinePatch> create();

    virtual ~MRNinePatch() = default;

    /// 设置九宫格源纹理（9 个象限共享同一纹理、不同 UV 切片）。
    void setTexture(TextureSharedPtr texture);

    /// 四边 inset（源纹理像素）。中间区域超出可拉伸范围时按比例钳制。
    void setInset(float left, float top, float right, float bottom);

    /// 四边同值 inset。
    void setInset(float inset);

    float getInsetLeft() const { return m_insetLeft; }
    float getInsetTop() const { return m_insetTop; }
    float getInsetRight() const { return m_insetRight; }
    float getInsetBottom() const { return m_insetBottom; }

    void update(FrameStateSharedPtr frameState) override;

private:
    void layoutChildren();

    std::array<MRImageSharedPtr, 9> m_cells;  // 行主序：左上…右下
    TextureSharedPtr m_texture;
    float m_insetLeft = 12.0f;
    float m_insetTop = 12.0f;
    float m_insetRight = 12.0f;
    float m_insetBottom = 12.0f;
    float m_textureWidth = 0.0f;
    float m_textureHeight = 0.0f;
};

using MRNinePatchSharedPtr = std::shared_ptr<MRNinePatch>;

}  // namespace morrow

#endif  // MORROW_MR_NINE_PATCH_H
