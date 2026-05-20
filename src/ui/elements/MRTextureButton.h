//
// Created by lance on 2025/10/2.
//

#ifndef MORROW_GUI_MRTEXTUREBUTTON_H
#define MORROW_GUI_MRTEXTUREBUTTON_H
#include "base/BaseButton.h"

namespace morrow {
class MRTextureButton : public BaseButton {
public:
    static std::shared_ptr<MRTextureButton> create();

    void setNormalTexture(TextureSharedPtr normalTexture);

    void setHoverTexture(TextureSharedPtr hoverTexture);

    void setPressedTexture(TextureSharedPtr pressedTexture);

    void setDisabledTexture(TextureSharedPtr disabledTexture);

    void setFocusedTexture(TextureSharedPtr focusedTexture);

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
