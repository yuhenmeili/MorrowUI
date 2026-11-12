//
// Created by 0060328 on 25-9-18.
//

#include "PlatformWindow.h"

#include "BatchManager.h"
#include "GlobalObject.h"

namespace morrow {
PlatformWindow::PlatformWindow() {
    m_batchManager = std::make_shared<BatchManager>();
    m_scene2D = std::make_shared<Scene2D>();
}

bool PlatformWindow::initializeIfNeeded() {
    return true;
}

bool PlatformWindow::isWindowShouldClose() {
    return false;
}

std::shared_ptr<Scene2D> PlatformWindow::scene2D() const {
    return m_scene2D;
}

void PlatformWindow::requestRender(const char* /*reason*/) {
    // 与原 UIWidget::requestRender 等价：唤醒按需渲染（GlobalObject 单例）。
    if (auto renderingThread = GlobalObject::getInstance().getRenderingThread()) {
        renderingThread->requestRender();
    }
}

void PlatformWindow::beginRenderPass(FrameStateSharedPtr frameState) {
    // 子类实现
}

void PlatformWindow::updateWidgets(FrameStateSharedPtr frameState) {
    m_scene2D->update(frameState);
}

void PlatformWindow::lateUpdateWidgets(FrameStateSharedPtr frameState) {
    m_scene2D->lateUpdate(frameState);
}

void PlatformWindow::commitRenderPass(FrameStateSharedPtr frameState) {
    // 子类实现
}

void PlatformWindow::setClearColor(float r, float g, float b, float a) {
}

void PlatformWindow::setCursorShape(CursorShape /*shape*/) {
}

WindowEvents& PlatformWindow::events() {
    return m_events;
}

void PlatformWindow::terminate() {
}

Vector2 PlatformWindow::framebufferSize() const {
    return {0.0f, 0.0f};
}

void PlatformWindow::setClipboardText(const std::string& /*text*/) {
}

std::string PlatformWindow::clipboardText() const {
    return {};
}
}
