//
// Created by 0060328 on 25-9-18.
//

#include "QNXEGLPlatform.h"

#include <algorithm>
#include <memory>

#include "GlobalObject.h"
#include "QNXInputProvider.h"
#include "../../utils/Log.h"

namespace morrow {
QNXEGLPlatform::QNXEGLPlatform(const WindowInfo& info) {
    m_requestedSamples = std::max(info.samples, 1);
    m_inputEventsManager = std::make_shared<InputEventsManager>();
    m_qnxInputProvider = std::make_shared<QNXInputProvider>();
    m_egl = std::make_shared<EGLOperationsQNX>(m_requestedSamples);
    m_egl->initialize();
    if (const auto context = m_egl->getContext()) {
        m_qnxInputProvider->setScreenContext(context->screen_ctx, context->screen_ev);
    }
    m_qnxInputProvider->setWindowOffset(info.x, info.y);
    m_inputEventsManager->setInputProvider(m_qnxInputProvider);
    m_window = EGLWindow::create(m_egl, info);
}

void QNXEGLPlatform::initialize(bool multithread) {
    Platform::initialize(multithread);
    m_window->initializeIfNeeded();
    ensureRenderCapabilitiesInitialized();
}

bool QNXEGLPlatform::beginFrame(FrameStateSharedPtr frameState) {
    const bool skipEvents = m_skipEventFrameCount > 0;
    m_qnxInputProvider->setSkipEvents(skipEvents);
    m_inputEventsManager->poll();
    if (skipEvents) {
        --m_skipEventFrameCount;
    }
    resolveInputTargets(frameState);
    return true;
}

void QNXEGLPlatform::beginRenderPass(FrameStateSharedPtr frameState) {
    m_window->beginRenderPass(frameState);
}

void QNXEGLPlatform::updateWidgets(FrameStateSharedPtr frameState) {
    m_window->updateWidgets(frameState);
}

void QNXEGLPlatform::commitRenderPass(FrameStateSharedPtr frameState) {
    m_window->commitRenderPass(frameState);
}

void QNXEGLPlatform::endFrame() {
    RENDERINGTHREAD->present(m_window->getSurface());
    RENDERINGTHREAD->endFrame();
}

void QNXEGLPlatform::terminate() {
    m_window->terminate();
}

InputEventsManagerSharedPtr QNXEGLPlatform::getInputManager() {
    return m_inputEventsManager;
}

ESContextSharedPtr QNXEGLPlatform::getContext() {
    return m_egl->getContext();
}
} // morrow

