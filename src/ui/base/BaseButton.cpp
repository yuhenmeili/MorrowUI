//
// Created by 0060328 on 25-10-14.
//

#include "BaseButton.h"

#include "Interaction.h"
#include "TouchEvent.h"

namespace morrow {
BaseButton::BaseButton() {
    // 默认挂载 Interaction 组件，按钮即可直接响应触摸事件
    m_interaction = addComponent<Interaction>();
    m_interaction->setClickEnabled(true);
    m_interaction->setLongPressEnabled(false);
    m_interaction->setInteractionEnabled(m_isEnabled && m_isInteractive);

    // TOUCH: 按下
    m_touchConnection = m_interaction->addEventListener(
        TOUCH_EVENT_TYPE_TOUCH,
        [this](TouchEvent& /*event*/) { onMouseDown(); });

    // MOVE: 用于按压过程中的进入/离开（可选）
    m_pointerEnterConnection = m_interaction->addEventListener(TOUCH_EVENT_TYPE_POINTER_ENTER, [this](TouchEvent& /*event*/) {
        if (!m_isHovered)
            onMouseEnter();
    });

    m_pointerLeaveConnection = m_interaction->addEventListener(TOUCH_EVENT_TYPE_POINTER_LEAVE, [this](TouchEvent& /*event*/) {
        if (m_isHovered)
            onMouseLeave();
    });

    // RELEASE: 只恢复状态，不触发 click
    m_releaseConnection = m_interaction->addEventListener(
        TOUCH_EVENT_TYPE_RELEASE,
        [this](TouchEvent& /*event*/) { onPointerRelease(); });

    // CLICK: 真正触发点击回调
    m_clickConnection = m_interaction->addEventListener(
        TOUCH_EVENT_TYPE_CLICK,
        [this](TouchEvent& /*event*/) { onPointerClick(); });
}

BaseButton::Events& BaseButton::events() {
    return m_events;
}

// 事件处理
void BaseButton::onMouseEnter() {
    if (!m_isEnabled || !m_isInteractive)
        return;

    m_isHovered = true;
    updateButtonState(ButtonState::HOVER);

    m_events.onPointerEntered.notify(*this);
}

void BaseButton::onMouseLeave() {
    if (!m_isEnabled || !m_isInteractive)
        return;

    m_isHovered = false;
    m_isPressed = false;
    updateButtonState(ButtonState::NORMAL);

    m_events.onPointerExited.notify(*this);
}

void BaseButton::onMouseDown() {
    LOG_I("Button {} MouseDown", getWidgetName());
    if (!m_isEnabled || !m_isInteractive)
        return;

    m_isPressed = true;
    updateButtonState(ButtonState::PRESSED);
    m_events.onPressed.notify(*this);
}

void BaseButton::onMouseUp() {
    if (!m_isEnabled || !m_isInteractive)
        return;

    // 保持旧语义：抬起会触发点击
    onPointerRelease();
    onPointerClick();
}

// 状态控制
void BaseButton::setEnabled(bool enabled) {
    if (m_isEnabled != enabled) {
        m_isEnabled = enabled;
        if (!enabled) {
            m_isHovered = false;
            m_isPressed = false;
        }
        updateButtonState(enabled ? ButtonState::NORMAL : ButtonState::DISABLED);
        if (m_interaction) {
            m_interaction->setInteractionEnabled(m_isEnabled && m_isInteractive);
        }
    }
}

void BaseButton::setInteractive(bool interactive) {
    m_isInteractive = interactive;
    if (!interactive) {
        m_isHovered = false;
        m_isPressed = false;
        updateButtonState(m_isEnabled ? ButtonState::NORMAL : ButtonState::DISABLED);
    }
    if (m_interaction) {
        m_interaction->setInteractionEnabled(m_isEnabled && m_isInteractive);
    }
}

void BaseButton::updateButtonState(ButtonState newState) {
    if (m_currentState != newState) {
        m_currentState = newState;
        updateVisualState();
        requestRender("button state");
    }
}

void BaseButton::updateVisualState() {
}

void BaseButton::onPointerRelease() {
    if (!m_isEnabled || !m_isInteractive)
        return;
    if (!m_isPressed)
        return;
    m_isPressed = false;
    updateButtonState(m_isHovered ? ButtonState::HOVER : ButtonState::NORMAL);
    m_events.onReleased.notify(*this);
}

void BaseButton::onPointerClick() {
    if (!m_isEnabled || !m_isInteractive)
        return;
    onActivated();
    m_events.onClicked.notify(*this);
}

void BaseButton::onActivated() {
}
}  // namespace morrow
