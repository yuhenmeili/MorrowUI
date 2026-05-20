#include "Engine.h"

#include <algorithm>

#include "FontManager.h"
#include "MathUtils.h"
#include "../ui/base/Widget.h"
#include "GlobalObject.h"
#include "PlatformFactory.h"
#include "../debug/DebugPlane.h"
#include "../ui/helpers/Tween.h"
#include "../renderer/RenderDeviceProxy.h"

namespace morrow {
Engine::Engine(const EngineOptions& options) {
    WindowInfo windowInfo = options.windowInfo;
    windowInfo.samples = std::max(options.samples, 1);

    LOG_I("multithread {}, enableRequestRender {}, samples {}",
          options.multithread,
          options.enableRequestRender,
          windowInfo.samples);
    m_requestRenderEnabled = options.enableRequestRender;
    m_platform = PlatformFactory::create(windowInfo);
    m_platform->initialize(options.multithread);

    m_camera = std::make_shared<OrthographicCamera>(0.0f, 11000.0f);
    m_camera->setPosition(0.0f, 0.0f, 10000.0f);
    m_camera->setDirection(0.0f, 0.0f, -1.0f);
    m_camera->setUp(0.0f, 1.0f, 0.0f);

    m_fpsController = std::make_shared<FPSController>(30);
    m_frameState = std::make_shared<FrameState>();
    // m_frameState->esContext = m_esContext;
    m_frameState->camera = m_camera;
    m_frameState->inputEventsManager = m_platform->getInputManager();
    m_debugPlane = std::make_shared<DebugPlane>();
    m_debugPlane->initialize(m_platform->getWindow());
}

Engine::~Engine() {
    GlobalObject::getInstance().destroy();
}

WindowSharedPtr Engine::getWindow() const {
    return m_platform ? m_platform->getWindow() : nullptr;
}

void Engine::addFonts(const std::vector<FontInfo>& fontsUrl) {
    GlobalObject::getInstance().getFontManager()->addFonts(fontsUrl);
}

void Engine::setFPS(int32_t fps) {
    m_fpsController->setFPS(fps);
}

Observable<>& Engine::preRender() {
    return m_preRender;
}

void Engine::render() {
    m_lastHeartbeatTime = Math::getCurrentMonotonicTime();
    while (!m_platform->shouldClose()) {
        updateFrameState();

        //event
        if (!m_platform->beginFrame(m_frameState)) {
            break;
        }
        m_preRender.notify();
        //prepare
        TweenManager::getInstance().update(m_frameState);

        //render
        // if (!m_requestRenderEnabled || (GlobalObject::getInstance().getRenderingThread()->isNeedRender() || !m_platform->getInputManager()->getInputEvents().empty())) {
            // GlobalObject::getInstance().getRenderingThread()->resetRenderStatus();
            m_platform->update(m_frameState);
            // LOG_I("drawcall count {}", m_frameState->drawCallCount);
            m_frameState->frameNumber++;
            //heartbeat
            heartbeat();
            //fps
            // m_fpsController->sleep();
            //debug
            m_debugPlane->update(m_frameState);
            //after render
            m_afterRender.notify();
            m_platform->endFrame();

            callAfterRenderFunctions();
        // }
    }

    m_platform->terminate();
}

Observable<>& Engine::afterRender() {
    return m_afterRender;
}

void Engine::updateFrameState() {
    double currentTime = Math::getCurrentMonotonicTime();
    if (m_monotonicTime <= 0) {
        //first frame
        m_monotonicTime = currentTime;
    }
    m_frameState->deltaTime = currentTime - m_monotonicTime;
    if (m_frameState->deltaTime < 0.0) m_frameState->deltaTime = 0.0;
    m_monotonicTime = currentTime;
    m_frameState->callAfterRender.clear();
    m_frameState->callAfterTouched.clear();
    m_frameState->debugWidgetsAfterAnimate.clear();
    m_frameState->drawCallCount = 0;
    m_frameState->batchManager = nullptr;
    m_frameState->ssboManager = GlobalObject::getInstance().getSSBOManager();
}

void Engine::callAfterRenderFunctions() {
    for (const auto& callback : m_frameState->callAfterRender) {
        callback();
    }
    for (const auto& widget : m_frameState->debugWidgetsAfterAnimate) {
        widget->debugTraversal("debugWidgetsAfterAnimate");
    }
}

void Engine::heartbeat() {
    double deltaTime = m_monotonicTime - m_lastHeartbeatTime;
    auto logGap = 2.0f;
    if (deltaTime >= logGap) {
        m_lastHeartbeatTime = m_monotonicTime;
        int32_t fps = 30;
        if (m_frameState->frameNumber >= m_lastHeartbeatFrameNumber) {
            uint32_t frameNumber = m_frameState->frameNumber - m_lastHeartbeatFrameNumber;
            fps = std::ceil((double)frameNumber / deltaTime);
        }
        m_lastHeartbeatFrameNumber = m_frameState->frameNumber;
        m_frameState->fps = fps;
        // LOG_I("Render FPS : {}", fps);
    } else if (deltaTime < 0) {
        m_lastHeartbeatTime = m_monotonicTime;
        LOG_E("Time hopping");
    }
}
}
