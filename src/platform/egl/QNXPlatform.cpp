//
// Created by 0060328 on 25-9-18.
//

#include "QNXPlatform.h"

#include <algorithm>
#include <memory>

#include "GlobalObject.h"
#include "QNXInputProvider.h"
#include "morrow/utils/Log.h"

namespace morrow {
QNXPlatform::QNXPlatform(const WindowInfo& info,
                         const std::vector<WindowInfo>& extraWindowInfos,
                         ScreenRenderMode screenMode) {
    m_requestedSamples = std::max(info.samples, 1);
    m_inputEventsManager = std::make_shared<InputEventsManager>();
    m_qnxInputProvider = std::make_shared<QNXInputProvider>();
    m_egl = std::make_shared<EGLOperationsQNX>(m_requestedSamples);
    m_egl->initialize();
    if (const auto context = m_egl->getContext()) {
        m_qnxInputProvider->setScreenContext(context->screen_ctx);
    }
    m_qnxInputProvider->setWindowOffset(info.x, info.y);
    // 独立 screen 事件线程：输入到达即唤醒主线程空闲等待（R1 方案 B）
    m_qnxInputProvider->startEventThread();
    m_inputEventsManager->setInputProvider(m_qnxInputProvider);

    std::vector<PlatformWindowSharedPtr> windows;
    windows.reserve(1 + extraWindowInfos.size());
    windows.push_back(EGLWindow::create(m_egl, info));
    for (const auto& extra : extraWindowInfos) {
        // 共享同一 EGL 上下文：镜像模式的内容 RT 纹理对各 surface 天然可见
        windows.push_back(EGLWindow::create(m_egl, extra));
    }
    setWindows(std::move(windows));

    m_mirrorActive = (screenMode == ScreenRenderMode::Mirror) && (getWindows().size() > 1);
    if (m_mirrorActive) {
        // 内容分辨率 = 主窗口尺寸（大屏 1:1 锐利，小屏贴图时等比缩小）
        m_presentStage = std::make_shared<PresentStage>();
        m_presentStage->setContentSize(info.width, info.height);
        LOG_I("mirror mode active: {} windows, content {}x{}",
              getWindows().size(), info.width, info.height);
    } else if (getWindows().size() > 1) {
        LOG_I("independent mode: {} windows render their own content", getWindows().size());
    }
}

void QNXPlatform::initialize(bool multithread, const RenderDeviceOptions& deviceOptions) {
    Platform::initialize(multithread, deviceOptions);
    for (const auto& window : getWindows()) {
        window->initializeIfNeeded();
    }
    // 多屏输入：注册窗口句柄表，采集线程按事件的 SCREEN_PROPERTY_WINDOW
    // 反查归属窗口（窗口创建时已各自绑定到指定 display）
    std::vector<QNXInputProvider::ScreenWindowSource> screenWindows;
    screenWindows.reserve(getWindows().size());
    for (const auto& window : getWindows()) {
        const auto eglWindow = std::dynamic_pointer_cast<EGLWindow>(window);
        if (!eglWindow || !eglWindow->isWindowInited()) {
            continue;
        }
        const Vector2 pos = eglWindow->getWindowPosition();
        screenWindows.push_back({eglWindow->getScreenWindow(),
                                 static_cast<int32_t>(pos.x),
                                 static_cast<int32_t>(pos.y)});
    }
    if (!screenWindows.empty()) {
        m_qnxInputProvider->setScreenWindows(screenWindows);
    }
    ensureRenderCapabilitiesInitialized();
}

bool QNXPlatform::beginFrame(FrameStateSharedPtr frameState) {
    const bool skipEvents = m_skipEventFrameCount > 0;
    m_qnxInputProvider->setSkipEvents(skipEvents);
    m_inputEventsManager->poll();
    if (skipEvents) {
        --m_skipEventFrameCount;
    }
    // 逐帧能力注入前移到输入阶段：Engine 的多窗口渲染循环直接调
    // window->beginRenderPass，不再经过 platform->beginRenderPass
    frameState->isSSBOSupport = m_isSSBOSupport;
    resolveInputTargets(frameState);
    return true;
}

void QNXPlatform::beginRenderPass(FrameStateSharedPtr frameState) {
    frameState->isSSBOSupport = m_isSSBOSupport;
    m_window->beginRenderPass(frameState);
}

void QNXPlatform::updateWidgets(FrameStateSharedPtr frameState) {
    m_window->updateWidgets(frameState);
}

void QNXPlatform::commitRenderPass(FrameStateSharedPtr frameState) {
    m_window->commitRenderPass(frameState);
}

void QNXPlatform::beginContentPass(FrameStateSharedPtr frameState) {
    if (m_mirrorActive) {
        m_presentStage->beginContentPass();
    }
}

void QNXPlatform::endContentPass(FrameStateSharedPtr frameState) {
    if (m_mirrorActive) {
        m_presentStage->endContentPass();
    }
}

void QNXPlatform::endFrame() {
    if (m_mirrorActive) {
        // 各屏贴图 + present（PresentStage 内部逐窗口 makeCurrent/clear/blit/present）
        m_presentStage->presentAll(getWindows());
    } else {
        // eglSwapBuffers 要求目标 surface 是调用线程当前的 draw surface，
        // 否则报 EGL_BAD_MATCH 且静默失败（多屏渲染顺序 A→B 结束时 current
        // 停留在最后一屏，先画的屏直接 swap 会上屏失败），所以逐窗口
        // 先 makeCurrent 再 present（与 PresentStage::presentAll 的顺序一致）
        for (const auto& window : getWindows()) {
            if (!window || !window->isWindowInited() || !window->isScreenVisible()) {
                continue;
            }
            RENDERINGTHREAD->makeCurrent(window->nativeSurface());
            RENDERINGTHREAD->present(window->nativeSurface());
        }
    }
    RENDERINGTHREAD->endFrame();
}

void QNXPlatform::waitForEvents(double timeoutSeconds) {
    // 空闲等待：阻塞至新输入唤醒（采集线程投递事件时 notify）或超时。
    // 输入与 worker 任务（requestRender → wakeEventLoop）不再受等待节拍限制。
    m_qnxInputProvider->waitForEventLoopWakeup(timeoutSeconds);
}

void QNXPlatform::wakeEventLoop() {
    m_qnxInputProvider->wakeEventLoop();
}

void QNXPlatform::terminate() {
    // PresentStage 的 RT/VBO 必须在渲染设备仍存活时释放（Engine::render 末尾
    // 统一 terminate，早于 GlobalObject::destroy）
    if (m_presentStage) {
        m_presentStage->releaseResources();
    }
    for (const auto& window : getWindows()) {
        window->terminate();
    }
}

InputEventsManagerSharedPtr QNXPlatform::getInputManager() {
    return m_inputEventsManager;
}

ESContextSharedPtr QNXPlatform::getContext() {
    return m_egl->getContext();
}
} // morrow
