//
// Created by 0060328 on 25-9-18.
//

#include "WGLPlatform.h"

#include <algorithm>

#include "GlobalObject.h"
#include "WGLWindow.h"
#include "WGLInputProvider.h"

namespace morrow {
WGLPlatform::WGLPlatform(const WindowInfo& info) {
    m_requestedSamples = std::max(info.samples, 1);
    m_inputManager = std::make_shared<InputEventsManager>();
    m_wglInputProvider = std::make_shared<WGLInputProvider>();
    m_inputManager->setInputProvider(m_wglInputProvider);
    m_window = std::make_shared<WGLWindow>(info);
    m_wglInputProvider->setWindow(m_window->getSurface());
};

void WGLPlatform::initialize(bool multithread) {
    Platform::initialize(multithread);
    m_window->initializeIfNeeded();
    RENDERINGTHREAD->makeCurrent(m_window->getSurface());
    ensureRenderCapabilitiesInitialized();
}

bool WGLPlatform::beginFrame(FrameStateSharedPtr frameState) {
    glfwPollEvents();
    m_inputManager->poll();
    resolveInputTargets(frameState);
    return true;
}

void WGLPlatform::beginRenderPass(FrameStateSharedPtr frameState) {
    m_window->beginRenderPass(frameState);
}

void WGLPlatform::updateWidgets(FrameStateSharedPtr frameState) {
    m_window->updateWidgets(frameState);
}

void WGLPlatform::commitRenderPass(FrameStateSharedPtr frameState) {
    m_window->commitRenderPass(frameState);
}

void WGLPlatform::endFrame() {
    RENDERINGTHREAD->present(m_window->getSurface());
    RENDERINGTHREAD->endFrame();
}

void WGLPlatform::terminate() {
    m_window->terminate();
}

InputEventsManagerSharedPtr WGLPlatform::getInputManager() {
    return m_inputManager;
}
} // morrow