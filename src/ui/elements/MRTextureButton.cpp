//
// Created by lance on 2025/10/2.
//

#include "MRTextureButton.h"

#include <utility>

#include "renderer/resource/ssbo/layouts/TextureButtonSSBOLayout.h"

namespace morrow {
std::shared_ptr<MRTextureButton> MRTextureButton::create() {
    return std::shared_ptr<MRTextureButton>(new MRTextureButton());
}

void MRTextureButton::setNormalTexture(TextureSharedPtr normalTexture) {
    m_normalTexture = std::move(normalTexture);
    updateVisualState();
    requestRender("setNormalTexture");
}

void MRTextureButton::setHoverTexture(TextureSharedPtr hoverTexture) {
    m_hoverTexture = std::move(hoverTexture);
    updateVisualState();
    requestRender("setHoverTexture");
}

void MRTextureButton::setPressedTexture(TextureSharedPtr pressedTexture) {
    m_pressedTexture = std::move(pressedTexture);
    updateVisualState();
    requestRender("setPressedTexture");
}

void MRTextureButton::setDisabledTexture(TextureSharedPtr disabledTexture) {
    m_disabledTexture = std::move(disabledTexture);
    updateVisualState();
    requestRender("setDisabledTexture");
}

void MRTextureButton::setFocusedTexture(TextureSharedPtr focusedTexture) {
    m_focusedTexture = std::move(focusedTexture);
    updateVisualState();
    requestRender("setFocusedTexture");
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
    if (!texture) {
        texture = m_normalTexture;
    }
    m_material->setTexture("texture", texture);
}

MRTextureButton::MRTextureButton() {
    setWidgetType("MRTextureButton");
    m_material->setShader("texture_button");
    m_material->setSSBOLayout(std::make_shared<TextureButtonSSBOLayout>());
}
} // morrow
