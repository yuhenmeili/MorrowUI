//
// Created by 0060328 on 25-9-18.
//

#include "morrow/Window.h"

#include "BatchManager.h"

namespace morrow {
Window::Window() : UIWidget(false) {
    m_batchManager = std::make_shared<BatchManager>();
}

bool Window::initializeIfNeeded() {
    return true;
}

bool Window::isWindowShouldClose() {
    return false;
}

void Window::beginRenderPass(FrameStateSharedPtr frameState) {
    // 子类实现
}

void Window::updateWidgets(FrameStateSharedPtr frameState) {
    UIWidget::update(frameState);
}

void Window::lateUpdateWidgets(FrameStateSharedPtr frameState) {
    UIWidget::lateUpdate(frameState);
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
