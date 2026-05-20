//
// Created by 0060328 on 25-9-18.
//

#ifndef WINDOWWINDOW_H
#define WINDOWWINDOW_H
#include "Window.h"
#include "OpenglHeader.h"

namespace morrow {

class WGLWindow: public Window{
public:
    explicit WGLWindow(const WindowInfo& info);

    bool initializeIfNeeded() override;

    void setClearColor(float r, float g, float b, float a) override;

    void update(FrameStateSharedPtr frameState) override;

    bool isWindowShouldClose() override;

    void terminate() override;

    void* getSurface() const override { return m_window; }

    Vector2 getFramebufferSize() const { return m_framebufferSize; }

private:
    static void window_size_callback(GLFWwindow *window, int width, int height);

    static void framebuffer_size_callback(GLFWwindow *window, int width, int height);

    static void mouse_callback(GLFWwindow *window, double xposIn, double yposIn);

    static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

    GLFWwindow* m_window = nullptr;
    Vector2 m_windowPosition;
    Vector2 m_windowSize;
    Vector2 m_framebufferSize;
    Vector4 m_clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
};

} // morrow

#endif //WINDOWWINDOW_H
