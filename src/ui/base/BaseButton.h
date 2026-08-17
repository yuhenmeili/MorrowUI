//
// Created by 0060328 on 25-10-14.
//

#ifndef BASEBUTTON_H
#define BASEBUTTON_H
#include "UIWidget.h"
#include <functional>
#include <cstdint>

namespace morrow {
class Interaction;
using ListenerHandle = uint64_t;
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
    BaseButton();

    // 事件回调
    void setOnClickCallback(std::function<void()> callback);

    void setOnHoverCallback(std::function<void()> callback);

    void setOnLeaveCallback(std::function<void()> callback);

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

    /// Interaction 事件桥接（RELEASE 只恢复状态；CLICK 才真正触发 onClick 回调）
    void onPointerRelease();
    void onPointerClick();

    // 事件回调
    std::function<void()> m_onClickCallback;
    std::function<void()> m_onHoverCallback;
    std::function<void()> m_onLeaveCallback;

    // Interaction（默认挂载）
    std::shared_ptr<Interaction> m_interaction;
    ListenerHandle m_touchHandle = 0;
    ListenerHandle m_pointerEnterHandle = 0;
    ListenerHandle m_pointerLeaveHandle = 0;
    ListenerHandle m_releaseHandle = 0;
    ListenerHandle m_clickHandle = 0;

    // 状态管理
    ButtonState m_currentState = ButtonState::NORMAL;
    bool m_isEnabled = true;//是否启用，控制外观变化
    bool m_isInteractive = true;//是否响应鼠标事件
    bool m_isHovered = false;
    bool m_isPressed = false;
};
} // morrow

#endif //BASEBUTTON_H
