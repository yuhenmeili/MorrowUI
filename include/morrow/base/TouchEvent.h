//
// Created by 0060328 on 2026/2/24.
//

#ifndef MORROW_GUI_TOUCHEVENT_H
#define MORROW_GUI_TOUCHEVENT_H
#include <cstdint>
#include <memory>
#include <vector>

namespace morrow {
class Widget;

enum TouchDeviceType : int32_t {
    TOUCH_DEVICE_TYPE_UNKNOWN = 0,
    TOUCH_DEVICE_TYPE_MOUSE = 1,
    TOUCH_DEVICE_TYPE_TOUCH = 2,
    TOUCH_DEVICE_TYPE_PEN = 3,
    TOUCH_DEVICE_TYPE_KEYBOARD = 4,
};

enum TouchKeyCode : int32_t {
    TOUCH_KEY_UNKNOWN = 0,
    TOUCH_KEY_BACKSPACE,
    TOUCH_KEY_DELETE,
    TOUCH_KEY_ENTER,
    TOUCH_KEY_LEFT,
    TOUCH_KEY_RIGHT,
    TOUCH_KEY_HOME,
    TOUCH_KEY_END,
    TOUCH_KEY_TAB,
    TOUCH_KEY_F3,
    TOUCH_KEY_A,
    TOUCH_KEY_C,
    // 桌面键盘扩展（追加在尾部，保持既有枚举值稳定）
    TOUCH_KEY_ESCAPE,
    TOUCH_KEY_UP,
    TOUCH_KEY_DOWN,
    TOUCH_KEY_S,
    TOUCH_KEY_Z,
    TOUCH_KEY_Y,
};

enum TouchMouseButton : int32_t {
    TOUCH_MOUSE_BUTTON_NONE = 0,
    TOUCH_MOUSE_BUTTON_LEFT = 1,
    TOUCH_MOUSE_BUTTON_RIGHT = 2,
    TOUCH_MOUSE_BUTTON_MIDDLE = 3,
};

enum TouchModifierFlags : uint32_t {
    TOUCH_MODIFIER_NONE = 0,
    TOUCH_MODIFIER_SHIFT = 1u << 0,
    TOUCH_MODIFIER_CTRL = 1u << 1,
    TOUCH_MODIFIER_ALT = 1u << 2,
};

enum TouchButtonFlags : uint32_t {
    TOUCH_BUTTON_FLAG_NONE = 0,
    TOUCH_BUTTON_FLAG_LEFT = 1u << 0,
    TOUCH_BUTTON_FLAG_RIGHT = 1u << 1,
    TOUCH_BUTTON_FLAG_MIDDLE = 1u << 2,
};

enum TouchEventType : int32_t {
    TOUCH_EVENT_TYPE_NONE = 0,
    TOUCH_EVENT_TYPE_TOUCH = 1,
    TOUCH_EVENT_TYPE_MOVE = 2,
    TOUCH_EVENT_TYPE_RELEASE = 3,
    TOUCH_EVENT_TYPE_TOUCH_AND_HOLD = 4,
    TOUCH_EVENT_TYPE_CLICK = 5,
    TOUCH_EVENT_TYPE_WHEEL = 6,
    TOUCH_EVENT_TYPE_POINTER_ENTER = 7,
    TOUCH_EVENT_TYPE_POINTER_LEAVE = 8,
    TOUCH_EVENT_TYPE_CHARACTER = 9,
    TOUCH_EVENT_TYPE_KEY_DOWN = 10,
};

struct TouchEvent {
    int32_t touchID = -1;  // 0,1,2...
    // 位置为世界系（中心原点、X 向右、Y 向上）：Platform::resolveInputTargets
    // 派发前已把平台屏幕像素统一转换，监听器与控件世界 AABB 直接同系使用。
    float positionX = 0.0f;
    float positionY = 0.0f;
    TouchEventType eventType = TOUCH_EVENT_TYPE_NONE;
    double touchTime = 0.0;
    int32_t totalTouchIDCount = 0;  // 当前帧touchid总数
    TouchDeviceType deviceType = TOUCH_DEVICE_TYPE_UNKNOWN;
    TouchMouseButton button = TOUCH_MOUSE_BUTTON_NONE;
    uint32_t buttonsMask = TOUCH_BUTTON_FLAG_NONE;
    uint32_t modifiers = TOUCH_MODIFIER_NONE;
    float wheelDeltaX = 0.0f;
    float wheelDeltaY = 0.0f;
    uint32_t unicodeCodepoint = 0;
    TouchKeyCode keyCode = TOUCH_KEY_UNKNOWN;
    uint32_t keyRepeatCount = 0;  // 键盘按键重复次数：首按下为 0，长按重复递增

    int32_t callbackTimes = 0;  // 回调触发次数， 长按回调用
    std::shared_ptr<Widget> target;

    TouchEvent& operator=(const TouchEvent& other) = default;
};
}  // namespace morrow
#endif  // MORROW_GUI_TOUCHEVENT_H
