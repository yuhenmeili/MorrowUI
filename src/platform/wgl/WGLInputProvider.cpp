//
// WGL 平台输入提供者：按当前窗口的鼠标状态生成 TOUCH / MOVE / RELEASE / WHEEL 事件。
//

#include "WGLInputProvider.h"

#include <unordered_map>

#include "Log.h"
#include "OpenglHeader.h"
#include "../../../math/MathUtils.h"
#include "../../../ui/base/TouchEvent.h"

namespace morrow {
namespace {
std::unordered_map<GLFWwindow*, WGLInputProvider*>& getProviderRegistry() {
    static std::unordered_map<GLFWwindow*, WGLInputProvider*> registry;
    return registry;
}

uint32_t queryModifierFlags(GLFWwindow* window) {
    uint32_t modifiers = TOUCH_MODIFIER_NONE;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
        modifiers |= TOUCH_MODIFIER_SHIFT;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
        modifiers |= TOUCH_MODIFIER_CTRL;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) {
        modifiers |= TOUCH_MODIFIER_ALT;
    }
    return modifiers;
}

uint32_t makeButtonsMask(bool leftPressed, bool rightPressed, bool middlePressed) {
    uint32_t mask = TOUCH_BUTTON_FLAG_NONE;
    if (leftPressed) {
        mask |= TOUCH_BUTTON_FLAG_LEFT;
    }
    if (rightPressed) {
        mask |= TOUCH_BUTTON_FLAG_RIGHT;
    }
    if (middlePressed) {
        mask |= TOUCH_BUTTON_FLAG_MIDDLE;
    }
    return mask;
}

int32_t pressedButtonCount(bool leftPressed, bool rightPressed, bool middlePressed) {
    return (leftPressed ? 1 : 0) + (rightPressed ? 1 : 0) + (middlePressed ? 1 : 0);
}
} // namespace

WGLInputProvider::~WGLInputProvider() {
    if (m_window) {
        getProviderRegistry().erase(m_window);
    }
}

void WGLInputProvider::setWindow(void* window) {
    m_window = static_cast<GLFWwindow*>(window);
    m_lastLeftPressed = false;
    m_lastRightPressed = false;
    m_lastMiddlePressed = false;
    m_accumulatedWheelX = 0.0;
    m_accumulatedWheelY = 0.0;

    if (m_window) {
        getProviderRegistry()[m_window] = this;
        glfwSetScrollCallback(m_window, WGLInputProvider::scroll_callback);
        double cursorX = 0.0;
        double cursorY = 0.0;
        glfwGetCursorPos(m_window, &cursorX, &cursorY);

        int windowWidth = 0;
        int windowHeight = 0;
        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetWindowSize(m_window, &windowWidth, &windowHeight);
        glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);

        const double scaleX = (windowWidth > 0) ? (double(framebufferWidth) / double(windowWidth)) : 1.0;
        const double scaleY = (windowHeight > 0) ? (double(framebufferHeight) / double(windowHeight)) : 1.0;
        m_lastX = cursorX * scaleX;
        m_lastY = cursorY * scaleY;
    }
}

void WGLInputProvider::poll(std::vector<TouchEvent>& outEvents) {
    outEvents.clear();
    if (!m_window) return;

    const bool leftPressed = (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    const bool rightPressed = (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
    const bool middlePressed = (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
    const uint32_t buttonsMask = makeButtonsMask(leftPressed, rightPressed, middlePressed);
    const uint32_t modifiers = queryModifierFlags(m_window);
    const int32_t totalTouchIDCount = pressedButtonCount(leftPressed, rightPressed, middlePressed);

    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(m_window, &x, &y);

    int windowWidth = 0;
    int windowHeight = 0;
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetWindowSize(m_window, &windowWidth, &windowHeight);
    glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);

    const double scaleX = (windowWidth > 0) ? (double(framebufferWidth) / double(windowWidth)) : 1.0;
    const double scaleY = (windowHeight > 0) ? (double(framebufferHeight) / double(windowHeight)) : 1.0;
    const auto fx = static_cast<float>(x * scaleX);
    const auto fy = static_cast<float>(y * scaleY);
    const bool moved = (fx != m_lastX || fy != m_lastY);
    const double now = Math::getCurrentMonotonicTime();

    auto emitButtonEvent = [&](bool pressed,
                               bool wasPressed,
                               int32_t touchID,
                               TouchMouseButton button) {
        TouchEventType eventType = TOUCH_EVENT_TYPE_NONE;
        if (pressed) {
            if (!wasPressed) {
                eventType = TOUCH_EVENT_TYPE_TOUCH;
            } else if (moved) {
                eventType = TOUCH_EVENT_TYPE_MOVE;
            }
        } else if (wasPressed) {
            eventType = TOUCH_EVENT_TYPE_RELEASE;
        }

        if (eventType == TOUCH_EVENT_TYPE_NONE) {
            return;
        }

        TouchEvent ev;
        ev.touchID = touchID;
        ev.positionX = fx;
        ev.positionY = fy;
        ev.eventType = eventType;
        ev.touchTime = now;
        ev.totalTouchIDCount = totalTouchIDCount;
        ev.deviceType = TOUCH_DEVICE_TYPE_MOUSE;
        ev.button = button;
        ev.buttonsMask = buttonsMask;
        ev.modifiers = modifiers;
        outEvents.push_back(ev);
    };

    emitButtonEvent(leftPressed, m_lastLeftPressed, 0, TOUCH_MOUSE_BUTTON_LEFT);
    emitButtonEvent(rightPressed, m_lastRightPressed, 1, TOUCH_MOUSE_BUTTON_RIGHT);
    emitButtonEvent(middlePressed, m_lastMiddlePressed, 2, TOUCH_MOUSE_BUTTON_MIDDLE);

    if (m_accumulatedWheelX != 0.0 || m_accumulatedWheelY != 0.0) {
        TouchEvent wheelEvent;
        wheelEvent.touchID = -1;
        wheelEvent.positionX = fx;
        wheelEvent.positionY = fy;
        wheelEvent.eventType = TOUCH_EVENT_TYPE_WHEEL;
        wheelEvent.touchTime = now;
        wheelEvent.totalTouchIDCount = totalTouchIDCount;
        wheelEvent.deviceType = TOUCH_DEVICE_TYPE_MOUSE;
        wheelEvent.button = TOUCH_MOUSE_BUTTON_NONE;
        wheelEvent.buttonsMask = buttonsMask;
        wheelEvent.modifiers = modifiers;
        wheelEvent.wheelDeltaX = static_cast<float>(m_accumulatedWheelX);
        wheelEvent.wheelDeltaY = static_cast<float>(m_accumulatedWheelY);
        outEvents.push_back(wheelEvent);
        m_accumulatedWheelX = 0.0;
        m_accumulatedWheelY = 0.0;
    }

    m_lastLeftPressed = leftPressed;
    m_lastRightPressed = rightPressed;
    m_lastMiddlePressed = middlePressed;
    m_lastX = fx;
    m_lastY = fy;
}

void WGLInputProvider::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    auto it = getProviderRegistry().find(window);
    if (it == getProviderRegistry().end() || !it->second) {
        return;
    }

    it->second->m_accumulatedWheelX += xoffset;
    it->second->m_accumulatedWheelY += yoffset;
}

} // namespace morrow
