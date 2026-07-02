//
// Created by 0060328 on 25-9-18.
//

#include "Window.h"

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

void Window::commitRenderPass(FrameStateSharedPtr frameState) {
    // 子类实现
}

void Window::setClearColor(float r, float g, float b, float a) {
}

void Window::terminate() {
}

void* Window::getSurface() const{
    return nullptr;
}
}
