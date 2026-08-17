//
// Created by lance on 2025/12/15.
//
#include "Interaction.h"

#include "TouchEvent.h"
#include "UIWidget.h"
#include "Widget.h"
#include "math/MathUtils.h"

namespace morrow {
void Interaction::setInteractionEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    if (!enabled) {
        reset();
    }
}

Rect Interaction::getHitTestRect() const {
    if (auto widget = getGameObject()) {
        auto uiWidget = std::dynamic_pointer_cast<UIWidget>(widget->shared_from_this());
        if (uiWidget) {
            return uiWidget->getScreenSpaceAABB();
        }
    }
    return Rect(0.0f, 0.0f, 0.0f, 0.0f);
}

bool Interaction::containsPoint(float x, float y) const {
    return getHitTestRect().Contains(x, y);
}

void Interaction::update(FrameStateSharedPtr frameState) {
    if (!m_enabled || !getGameObject() || !getGameObject()->getVisible()) {
        if (m_isPressed) reset();
        return;
    }
    checkLongPress(frameState);
}

// 更推荐的事件入口（由输入系统调用）
void Interaction::handleTouchEvent(TouchEvent& event) {
    if (!m_enabled) return;

    switch (event.eventType) {
        case TOUCH_EVENT_TYPE_TOUCH:
            if (!m_isPressed) {
                const bool isCurrentTarget = event.target && getGameObject() && event.target.get() == getGameObject();
                if (!isCurrentTarget && !containsPoint(event.positionX, event.positionY)) {
                    break;
                }
                m_isPressed = true;
                m_activeTouchID = event.touchID;
                m_pressStartTime = event.touchTime;
                m_pressPosition = Vector2(event.positionX, event.positionY);
                m_longPressEventTriggered = false;
                dispatchEvent(TOUCH_EVENT_TYPE_TOUCH, event);
            }
            break;

        case TOUCH_EVENT_TYPE_MOVE:
            // MOVE is also used for mouse hover. Pressed-pointer capture is
            // still preserved for drag gestures, but an unpressed move must
            // reach the current hit target as well.
            if (!m_isPressed || event.touchID == m_activeTouchID) {
                dispatchEvent(TOUCH_EVENT_TYPE_MOVE, event);
            }
            break;

        case TOUCH_EVENT_TYPE_POINTER_ENTER:
            dispatchEvent(TOUCH_EVENT_TYPE_POINTER_ENTER, event);
            break;

        case TOUCH_EVENT_TYPE_POINTER_LEAVE:
            dispatchEvent(TOUCH_EVENT_TYPE_POINTER_LEAVE, event);
            break;

        case TOUCH_EVENT_TYPE_RELEASE:
            if (m_isPressed && event.touchID == m_activeTouchID) {
                m_isPressed = false;
                m_activeTouchID = -1;
                dispatchEvent(TOUCH_EVENT_TYPE_RELEASE, event);

                // 触发 Click（如果位置偏移不大）
                if (m_clickEnabled) {
                    float dx = event.positionX - m_pressPosition.x;
                    float dy = event.positionY - m_pressPosition.y;
                    if (dx * dx + dy * dy < 25.0f) {
                        // 5px 内
                        dispatchEvent(TOUCH_EVENT_TYPE_CLICK, event);
                    }
                }
            }
            break;

        default:
            break;
    }
}

void Interaction::reset() {
    m_isPressed = false;
    m_activeTouchID = -1;
    m_longPressEventTriggered = false;
}

void Interaction::setClickEnabled(bool enabled) {
    m_clickEnabled = enabled;
}

void Interaction::setLongPressEnabled(bool enabled) {
    m_longPressEnabled = enabled;
}

void Interaction::setLongPressDelay(float seconds) {
    m_longPressDelay = seconds;
}

void Interaction::checkLongPress(FrameStateSharedPtr /*frameState*/) {
    if (!m_longPressEnabled || !m_isPressed || m_longPressEventTriggered) return;
    double now = Math::getCurrentMonotonicTime();
    if (now - m_pressStartTime >= static_cast<double>(m_longPressDelay)) {
        m_longPressEventTriggered = true;
        TouchEvent holdEvent;
        holdEvent.touchID = m_activeTouchID;
        holdEvent.positionX = m_pressPosition.x;
        holdEvent.positionY = m_pressPosition.y;
        holdEvent.eventType = TOUCH_EVENT_TYPE_TOUCH_AND_HOLD;
        holdEvent.touchTime = now;
        if (getGameObject()) holdEvent.target = getGameObject()->shared_from_this();
        dispatchEvent(TOUCH_EVENT_TYPE_TOUCH_AND_HOLD, holdEvent);
    }
}
} // namespace morrow
