//
// Created by lance on 2025/10/2.
//

#include "MRTextureButton.h"

namespace morrow {
std::shared_ptr<MRTextureButton> MRTextureButton::create() {
    return std::shared_ptr<MRTextureButton>(new MRTextureButton());
}

void MRTextureButton::setNormalTexture(TextureSharedPtr normalTexture) {
    m_normalTexture = normalTexture;
}

void MRTextureButton::setHoverTexture(TextureSharedPtr hoverTexture) {
    m_hoverTexture = hoverTexture;
}

void MRTextureButton::setPressedTexture(TextureSharedPtr pressedTexture) {
    m_pressedTexture = pressedTexture;
}

void MRTextureButton::setDisabledTexture(TextureSharedPtr disabledTexture) {
    m_disabledTexture = disabledTexture;
}

void MRTextureButton::setFocusedTexture(TextureSharedPtr focusedTexture) {
    m_focusedTexture = focusedTexture;
}

void MRTextureButton::update(FrameStateSharedPtr frameState) {
    BaseButton::update(frameState);
}

void MRTextureButton::updateVisualState() {
    auto texture = m_normalTexture;
    switch (m_currentState) {
        case ButtonState::NORMAL:
            texture = m_normalTexture;
            break;
        case ButtonState::HOVER:
            texture = m_hoverTexture;
            break;
        case ButtonState::PRESSED:
            texture = m_pressedTexture;
            break;
        case ButtonState::DISABLED:
            texture = m_disabledTexture;
            break;
        case ButtonState::FOCUSED:
            texture = m_focusedTexture;
            break;
    }
    m_material->setTexture("texture", texture);
}

MRTextureButton::MRTextureButton() {
    setWidgetType("MRTextureButton");
    m_material->setShader("texture_button");
}
} // morrow
