//
// Created by 0060328 on 25-10-14.
//

#ifndef BASEBUTTON_H
#define BASEBUTTON_H
#include <cstdint>

#include "EventDispatcher.h"
#include "UIWidget.h"

namespace morrow {
class Interaction;
// 按钮状态枚举
enum class ButtonState {
    NORMAL,
    HOVER,
    PRESSED,
    DISABLED,
    FOCUSED,
};

class BaseButton : public UIWidget {
public:
    struct Events {
        Observable<BaseButton&> onClicked;
        Observable<BaseButton&> onPointerEntered;
        Observable<BaseButton&> onPointerExited;
        Observable<BaseButton&> onPressed;
        Observable<BaseButton&> onReleased;
    };

    BaseButton();

    Events& events();

    // 事件处理
    void onMouseEnter();

    void onMouseLeave();

    void onMouseDown();

    void onMouseUp();

    // 状态控制
    void setEnabled(bool enabled);

    void setInteractive(bool interactive);

protected:
    void updateButtonState(ButtonState newState);

    virtual void updateVisualState();

    /// Called after a valid click and before the user click callback.
    /// Stateful buttons can use this hook without replacing the callback.
    virtual void onActivated();

    /// Interaction 事件桥接（RELEASE 只恢复状态；CLICK 才真正触发 onClick 回调）
    void onPointerRelease();

    void onPointerClick();

    Events m_events;

    // Interaction（默认挂载）
    std::shared_ptr<Interaction> m_interaction;
    EventConnection m_touchConnection;
    EventConnection m_pointerEnterConnection;
    EventConnection m_pointerLeaveConnection;
    EventConnection m_releaseConnection;
    EventConnection m_clickConnection;

    // 状态管理
    ButtonState m_currentState = ButtonState::NORMAL;
    bool m_isEnabled = true;      // 是否启用，控制外观变化
    bool m_isInteractive = true;  // 是否响应鼠标事件
    bool m_isHovered = false;
    bool m_isPressed = false;
};
}  // namespace morrow

#endif  // BASEBUTTON_H
