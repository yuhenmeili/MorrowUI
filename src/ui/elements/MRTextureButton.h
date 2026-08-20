//
// Created by lance on 2025/10/2.
//

#ifndef MORROW_GUI_MRTEXTUREBUTTON_H
#define MORROW_GUI_MRTEXTUREBUTTON_H
#include "base/BaseButton.h"

namespace morrow {
/// 根据交互状态切换纹理的图片按钮组件。
class MRTextureButton : public BaseButton {
public:
    /// 创建一个纹理按钮。
    static std::shared_ptr<MRTextureButton> create();

    /// 设置默认状态纹理。
    void setNormalTexture(TextureSharedPtr normalTexture);

    /// 设置悬停状态纹理。
    void setHoverTexture(TextureSharedPtr hoverTexture);

    /// 设置按下状态纹理。
    void setPressedTexture(TextureSharedPtr pressedTexture);

    /// 设置禁用状态纹理。
    void setDisabledTexture(TextureSharedPtr disabledTexture);

    /// 设置获得焦点时使用的纹理。
    void setFocusedTexture(TextureSharedPtr focusedTexture);

    /// 每帧更新当前交互状态对应的纹理绘制。
    void update(FrameStateSharedPtr frameState) override;

protected:
    void updateVisualState() override;

private:
    MRTextureButton();

    TextureSharedPtr m_normalTexture;
    TextureSharedPtr m_hoverTexture;
    TextureSharedPtr m_pressedTexture;
    TextureSharedPtr m_disabledTexture;
    TextureSharedPtr m_focusedTexture;
};
} // morrow

#endif //MORROW_GUI_MRTEXTUREBUTTON_H
