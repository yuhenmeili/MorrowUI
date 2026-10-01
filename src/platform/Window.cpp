//
// Created by 0060328 on 25-9-18.
//

#include "Window.h"

#include "BatchManager.h"
#include "GlobalObject.h"

namespace morrow {
Window::Window() {
    m_batchManager = std::make_shared<BatchManager>();
    m_scene2D = std::make_shared<Scene2D>();
}

bool Window::initializeIfNeeded() {
    return true;
}

bool Window::isWindowShouldClose() {
    return false;
}

std::shared_ptr<Scene2D> Window::scene2D() const {
    return m_scene2D;
}

void Window::requestRender(const char* /*reason*/) {
    // 与原 UIWidget::requestRender 等价：唤醒按需渲染（GlobalObject 单例）。
    if (auto renderingThread = GlobalObject::getInstance().getRenderingThread()) {
        renderingThread->requestRender();
    }
}

void Window::beginRenderPass(FrameStateSharedPtr frameState) {
    // 子类实现
}

void Window::updateWidgets(FrameStateSharedPtr frameState) {
    m_scene2D->update(frameState);
}

void Window::lateUpdateWidgets(FrameStateSharedPtr frameState) {
    m_scene2D->lateUpdate(frameState);
}

void Window::commitRenderPass(FrameStateSharedPtr frameState) {
    // 子类实现
}

void Window::setClearColor(float r, float g, float b, float a) {
}

void Window::setCursorShape(CursorShape /*shape*/) {
}

WindowEvents& Window::events() {
    return m_events;
}

void Window::terminate() {
}

Vector2 Window::framebufferSize() const {
    return {0.0f, 0.0f};
}

void Window::setClipboardText(const std::string& /*text*/) {
}

std::string Window::clipboardText() const {
    return {};
}
}
