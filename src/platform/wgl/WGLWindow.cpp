//
// Created by 0060328 on 25-9-18.
//

#include "WGLWindow.h"

#include "BatchManager.h"
#include "GlobalObject.h"
#include "OrthographicCamera.h"
#include "base/Transform.h"
#include "utils/Log.h"

namespace morrow {
WGLWindow::WGLWindow(const WindowInfo& info) {
    setWidgetType("MRWindow");
    if (!glfwInit()) {
        LOG_E("Failed to initialize GLFW");
        return;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, info.samples > 1 ? info.samples : 0);

    m_window = glfwCreateWindow(info.width, info.height, "morrow", nullptr, nullptr);
    if (m_window == nullptr) {
        LOG_E("Failed to create GLFW window");
        glfwTerminate();
        return;
    }
    glfwSetWindowSizeCallback(m_window, window_size_callback);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);
    glfwSetWindowContentScaleCallback(m_window, content_scale_callback);
    glfwSetCursorPosCallback(m_window, mouse_callback);
    glfwSetScrollCallback(m_window, scroll_callback);
    glfwSetMouseButtonCallback(m_window, mouse_button_callback);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetWindowUserPointer(m_window, this);
    m_windowPosition.set(static_cast<float>(info.x), static_cast<float>(info.y));
    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(m_window, &windowWidth, &windowHeight);
    m_windowSize.set(static_cast<float>(windowWidth), static_cast<float>(windowHeight));
    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
    m_framebufferSize.set(static_cast<float>(fbWidth), static_cast<float>(fbHeight));
    float xscale = 1.0f;
    float yscale = 1.0f;
    glfwGetWindowContentScale(m_window, &xscale, &yscale);
    m_contentScale = std::max(xscale, yscale);
}

bool WGLWindow::initializeIfNeeded() {
    RENDERINGTHREAD->makeCurrent(m_window);
    return true;
}

void WGLWindow::setClearColor(float r, float g, float b, float a) {
    if (m_clearColor.x != r ||
        m_clearColor.y != g ||
        m_clearColor.z != b ||
        m_clearColor.w != a) {
        m_clearColor.set(r, g, b, a);
        requestRender("setClearColor");
    }
}

void WGLWindow::setCursorShape(CursorShape shape) {
    if (!m_window)
        return;
    int glfwShape = GLFW_ARROW_CURSOR;
    switch (shape) {
        case CursorShape::IBeam:
            glfwShape = GLFW_IBEAM_CURSOR;
            break;
        case CursorShape::Hand:
            glfwShape = GLFW_HAND_CURSOR;
            break;
        case CursorShape::ResizeHorizontal:
            glfwShape = GLFW_HRESIZE_CURSOR;
            break;
        case CursorShape::ResizeVertical:
            glfwShape = GLFW_VRESIZE_CURSOR;
            break;
        case CursorShape::ResizeAll:
            glfwShape = GLFW_CROSSHAIR_CURSOR;
            break;
        case CursorShape::Forbidden:
            glfwShape = GLFW_NOT_ALLOWED_CURSOR;
            break;
        case CursorShape::Arrow:
            break;
    }
    auto& cursor = m_cursors[glfwShape];
    if (!cursor)
        cursor = glfwCreateStandardCursor(glfwShape);
    glfwSetCursor(m_window, cursor);
}

void WGLWindow::beginRenderPass(FrameStateSharedPtr frameState) {
    int windowWidth = 0, windowHeight = 0;
    glfwGetWindowSize(m_window, &windowWidth, &windowHeight);
    if (windowWidth > 0 && windowHeight > 0) {
        m_windowSize.set(static_cast<float>(windowWidth), static_cast<float>(windowHeight));
    }

    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
    if (fbWidth > 0 && fbHeight > 0) {
        m_framebufferSize.set(static_cast<float>(fbWidth), static_cast<float>(fbHeight));
    }

    auto transform = getComponent<Transform>();
    if (transform) {
        const Vector3 size = transform->getSize();
        if (size.x != m_framebufferSize.x || size.y != m_framebufferSize.y) {
            transform->setSize(m_framebufferSize.x, m_framebufferSize.y);
        }
    }

    frameState->screenAlpha = 1.0f;
    frameState->framebufferWidth = fbWidth;
    frameState->framebufferHeight = fbHeight;
    frameState->currentClip = {};
    frameState->clipStack.clear();
    frameState->camera->update(m_windowPosition.x, m_windowPosition.y, m_framebufferSize.x, m_framebufferSize.y);
    frameState->batchManager = m_batchManager;
    RENDERINGTHREAD->setClearColor(m_clearColor.x, m_clearColor.y, m_clearColor.z, m_clearColor.w);
    RENDERINGTHREAD->setViewPort(0, 0, fbWidth, fbHeight);
    RENDERINGTHREAD->setScissorRect(false, 0, 0, 0, 0);
    RENDERINGTHREAD->clear();
    m_batchManager->clear();
}

void WGLWindow::updateWidgets(FrameStateSharedPtr frameState) {
    Window::updateWidgets(frameState);
}

void WGLWindow::commitRenderPass(FrameStateSharedPtr frameState) {
    m_batchManager->renderBatches(frameState);
}

bool WGLWindow::isWindowShouldClose() {
    return glfwWindowShouldClose(m_window);
}

void WGLWindow::terminate() {
    for (auto& [shape, cursor] : m_cursors) {
        (void)shape;
        if (cursor)
            glfwDestroyCursor(cursor);
    }
    m_cursors.clear();
    glfwDestroyWindow(m_window);
    m_window = nullptr;
    glfwTerminate();
}

void WGLWindow::window_size_callback(GLFWwindow* window, int width, int height) {
    if (!window) return;
    auto* self = static_cast<WGLWindow*>(glfwGetWindowUserPointer(window));
    if (!self) return;
    self->m_windowSize.set(static_cast<float>(width), static_cast<float>(height));
    self->m_events.onWindowSizeChanged.notify(self->m_windowSize);
    self->requestRender("window_size_callback");
}

void WGLWindow::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    if (!window) return;
    auto* self = static_cast<WGLWindow*>(glfwGetWindowUserPointer(window));
    if (!self) return;
    self->m_framebufferSize.set(static_cast<float>(width), static_cast<float>(height));
    self->m_events.onFramebufferSizeChanged.notify(self->m_framebufferSize);
    self->requestRender("framebuffer_size_callback");
}

void WGLWindow::content_scale_callback(GLFWwindow* window, float xscale, float yscale) {
    if (!window)
        return;
    auto* self = static_cast<WGLWindow*>(glfwGetWindowUserPointer(window));
    if (!self)
        return;
    self->m_contentScale = std::max(xscale, yscale);
    self->m_events.onContentScaleChanged.notify(self->m_contentScale);
    self->requestRender("content_scale_callback");
}

void WGLWindow::mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
}

void WGLWindow::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
}

void WGLWindow::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
}
} // morrow
