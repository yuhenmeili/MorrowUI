//
// WGL 平台输入提供者：将 GLFW 鼠标事件转换为 TouchEvent 流，供 InputEventsManager 每帧 poll。
//

#ifndef MORROW_WGL_INPUT_PROVIDER_H
#define MORROW_WGL_INPUT_PROVIDER_H

#include "platform/InputProvider.h"

struct GLFWwindow;

namespace morrow {

class WGLInputProvider : public IInputProvider {
public:
    WGLInputProvider() = default;

    ~WGLInputProvider() override;

    void poll(std::vector<TouchEvent>& outEvents) override;

    void setWindow(void* window);

private:
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

    static void character_callback(GLFWwindow* window, unsigned int codepoint);

    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

    GLFWwindow* m_window = nullptr;
    bool m_lastLeftPressed = false;
    bool m_lastRightPressed = false;
    bool m_lastMiddlePressed = false;
    double m_lastX = 0.0;
    double m_lastY = 0.0;
    double m_accumulatedWheelX = 0.0;
    double m_accumulatedWheelY = 0.0;
    std::vector<TouchEvent> m_pendingKeyboardEvents;
};

}  // namespace morrow

#endif  // MORROW_WGL_INPUT_PROVIDER_H
