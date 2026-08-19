//
// Created by lance on 2025/12/15.
//

#ifndef MORROW_GUI_INTERACTIONCOMPONENT_H
#define MORROW_GUI_INTERACTIONCOMPONENT_H
#include "Component.h"
#include "EventDispatcher.h"
#include "Rect.h"

namespace morrow {
using namespace Math;
class Widget;
struct TouchEvent;

class Interaction : public Component, public EventDispatcher {
public:
    Interaction() = default;

    ~Interaction() override = default;

    // 是否启用交互（默认 true）
    void setInteractionEnabled(bool enabled);

    bool isInteractionEnabled() const {
        return m_enabled;
    }

    // 自定义命中测试区域（默认使用 Widget 的包围盒）
    virtual Rect getHitTestRect() const;

    // 检查点是否在交互区域内
    virtual bool containsPoint(float x, float y) const;

    // ------------------ 行为配置 ------------------
    void setClickEnabled(bool enabled);

    void setLongPressEnabled(bool enabled);

    void setLongPressDelay(float seconds);  // 默认 0.5f

    void setKeyboardFocusable(bool focusable);

    bool isKeyboardFocusable() const {
        return m_keyboardFocusable;
    }

    // ------------------ 内部更新（由引擎每帧调用） ------------------
    void update(FrameStateSharedPtr frameState) override;

    /// 由事件系统/Widget::dispatchTouchEvent 调用，处理原始触摸事件并派发到监听器
    void handleTouchEvent(TouchEvent& event);

    // ------------------ 重置状态（如 Widget 隐藏时调用） ------------------
    void reset();

private:
    void checkLongPress(FrameStateSharedPtr frameState);

private:
    bool m_enabled = true;
    bool m_clickEnabled = true;
    bool m_longPressEnabled = false;
    bool m_keyboardFocusable = false;
    float m_longPressDelay = 0.5f;  // 秒

    // 状态跟踪
    bool m_isPressed = false;
    int32_t m_activeTouchID = -1;
    double m_pressStartTime = 0.0;
    Vector2 m_pressPosition = {0.0f, 0.0f};
    double m_lastUpdateTime = 0.0;

    // 长按计时器（避免重复触发）
    bool m_longPressEventTriggered = false;
};
}  // namespace morrow

#endif  // MORROW_GUI_INTERACTIONCOMPONENT_H
