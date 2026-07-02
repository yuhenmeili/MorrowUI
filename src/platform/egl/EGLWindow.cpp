//
// Created by lance on 2023/7/18.
//

#include "EGLWindow.h"
#include "BatchManager.h"
#include "utils/Log.h"
#include "GlobalObject.h"
#include "OrthographicCamera.h"
#include "base/Transform.h"

namespace morrow
{
EGLWindowSharedPtr EGLWindow::create(EGLOperationsQNXSharedPtr platform, const WindowInfo& info)
{
    return std::shared_ptr<EGLWindow>(new EGLWindow(platform, info));
}

EGLWindow::EGLWindow(EGLOperationsQNXSharedPtr platform, const WindowInfo& windowInfo)
{
    m_widgetName = windowInfo.name;
    m_displayId = windowInfo.displayId;
    m_zorder = windowInfo.zorder;
    m_windowAlpha = static_cast<int32_t>(windowInfo.alpha * 255.0f);
    m_winSensitivity = windowInfo.sensitivity;

    m_windowPosition.x = float(windowInfo.x);
    m_windowPosition.y = float(windowInfo.y);
    m_windowSize.x = float(windowInfo.width);
    m_windowSize.y = float(windowInfo.height);

    m_widgetType = "MRWindow";
    m_platform = platform;
}

EGLWindow::~EGLWindow()
{
    terminate();
}

bool EGLWindow::initializeIfNeeded() {
    while (!m_windowInited && m_initAttemptCounts <= 3) {
        initialize();
    }
    return m_windowInited;
}

void EGLWindow::initialize()
{
    if(m_initAttemptCounts <= 3) {
        if (EXIT_SUCCESS == initWindow() && EXIT_SUCCESS == initDisplay() && EXIT_SUCCESS == initSurface()) {
            m_windowInited = true;
        } else {
            LOG_I("window {} init failed", m_widgetName.c_str());
        }
        m_initAttemptCounts++;
    }
}

int32_t EGLWindow::initWindow()
{
    int32_t rc = screen_create_window(&screen_win, m_platform->getContext()->screen_ctx);
    if (rc) {
        LOG_I("screen_create_window failed");
        return EXIT_FAILURE;
    }

    int32_t setWindowPos[2] = {int32_t(m_windowPosition.x), int32_t(m_windowPosition.y)};
    int32_t setWindowSize[2] = {int32_t(m_windowSize.x), int32_t(m_windowSize.y)};

    screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_POSITION, setWindowPos);
    screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_SIZE, setWindowSize);
    screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_GLOBAL_ALPHA, &m_windowAlpha);
    screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_ALPHA_MODE, &m_windowAlphaMode);
    screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_ZORDER, &m_zorder);
    if (m_winSensitivity != WindowMask::DEFAULT_MASK) {
        auto eglSensitivity = getWindowSensitivity(m_winSensitivity);
        screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_SENSITIVITY, &eglSensitivity);
    }
    screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_FORMAT, (const int32_t[]) {SCREEN_FORMAT_RGBA8888});
    screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_USAGE, (const int32_t[]) {SCREEN_USAGE_OPENGL_ES3});

    rc = screen_create_window_buffers(screen_win, 2);
    if (rc) {
        LOG_I("screen_create_window_buffers failed");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int32_t EGLWindow::initDisplay()
{
    int32_t ndisplays = 0;
    int32_t displayId = 0;
    int32_t displaySize[2] = {0, 0};
    screen_get_context_property_iv(m_platform->getContext()->screen_ctx, SCREEN_PROPERTY_DISPLAY_COUNT, &ndisplays);
    if (!ndisplays) {
        LOG_I("screen_get_context_property_iv failed");
        return EXIT_FAILURE;
    }
    screen_display_t screen_display = nullptr;
    screen_display_array = (screen_display_t*) calloc(ndisplays, sizeof(screen_display_t));
    screen_get_context_property_pv(m_platform->getContext()->screen_ctx, SCREEN_PROPERTY_DISPLAYS, (void**) screen_display_array);
    for (int32_t i = 0; i < ndisplays; i++) {
        screen_get_display_property_iv(screen_display_array[i], SCREEN_PROPERTY_ID, &displayId);
        screen_get_display_property_iv(screen_display_array[i], SCREEN_PROPERTY_SIZE, displaySize);

       LOG_I("TargetDisplayId={}, displayId: {}, displaySize.x: {}, displaySize.y: {}", m_displayId, displayId, displaySize[0], displaySize[1]);

        if (m_displayId == displayId) {
            screen_display = screen_display_array[i];
        }
    }
    if (!screen_display) {
        LOG_I("!m_display failed");
        return EXIT_FAILURE;
    }
    screen_set_window_property_pv(screen_win, SCREEN_PROPERTY_DISPLAY, (void**) &screen_display);
    return EXIT_SUCCESS;
}

int32_t EGLWindow::initSurface()
{
    const EGLint egl_surface_attrib_list[] = {
        EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
        EGL_NONE
    };

    m_eglSurface = eglCreateWindowSurface(m_platform->getContext()->eglDisplay, m_platform->getContext()->egl_conf, (EGLNativeWindowType) screen_win, egl_surface_attrib_list);
    if (m_eglSurface == EGL_NO_SURFACE) {
        LOG_I("eglCreateWindowSurface failed");
        return EXIT_FAILURE;
    }
    RENDERINGTHREAD->makeCurrent(m_eglSurface);
    return EXIT_SUCCESS;
}

void EGLWindow::terminate()
{
    RENDERINGTHREAD->makeCurrent(nullptr);
    if (m_eglSurface) {
        eglDestroySurface(m_platform->getContext()->eglDisplay, m_eglSurface);
        m_eglSurface = nullptr;
    }
    if (screen_win) {
        screen_destroy_window_buffers(screen_win);
        screen_destroy_window(screen_win);
        screen_win = nullptr;
    }
    if (screen_display_array) {
        free(screen_display_array);
        screen_display_array = nullptr;
    }
    m_windowInited = false;
    m_initAttemptCounts = 0;
}

int32_t EGLWindow::getWindowSensitivity(WindowMask mask) {
    if (mask == WindowMask::CONTINUE_MASK) {
        return SCREEN_SENSITIVITY_MASK_CONTINUE;
    }
    else if (mask == WindowMask::ALWAYS_MASK) {
        return SCREEN_SENSITIVITY_MASK_ALWAYS;
    }
    else if (mask == WindowMask::NEVER_MASK) {
        return SCREEN_SENSITIVITY_MASK_NEVER;
    }
    return 0;
}

void EGLWindow::setClearColor(float r, float g, float b, float a)
{
    if(m_clearColor.x != r || m_clearColor.y != g || m_clearColor.z != b || m_clearColor.w != a) {
        m_clearColor.set(r, g, b, a);
        LOG_I("{}, {}, {}, {}, {}", this->getIdentityInfo().c_str(), r, g, b, a);
        requestRender("setClearColor");
    }
}

void EGLWindow::setScreenAlpha(float alpha)
{
    if (m_screenAlpha != alpha) {
        m_screenAlpha = alpha;
        LOG_I("{}, {}", this->getIdentityInfo(), alpha);
        requestRender( "setScreenAlpha");
    }
}

void EGLWindow::beginRenderPass(FrameStateSharedPtr frameState)
{
    if (!m_windowInited) return;

    auto transform = getComponent<Transform>();
    if (transform) {
        const Vector3 size = transform->getSize();
        if (size.x != m_windowSize.x || size.y != m_windowSize.y) {
            transform->setSize(m_windowSize.x, m_windowSize.y);
        }
    }

    frameState->screenAlpha = m_screenAlpha;
    frameState->camera->update(m_windowPosition.x, m_windowPosition.y, m_windowSize.x, m_windowSize.y);
    frameState->batchManager = m_batchManager;
    RENDERINGTHREAD->setClearColor(m_clearColor.x, m_clearColor.y, m_clearColor.z, m_clearColor.w);
    RENDERINGTHREAD->setViewPort(0, 0, int32_t(m_windowSize.x), int32_t(m_windowSize.y));
    RENDERINGTHREAD->clear();
    m_batchManager->clear();
}

void EGLWindow::updateWidgets(FrameStateSharedPtr frameState)
{
    if (m_windowInited) {
        Window::updateWidgets(frameState);
    }
}

void EGLWindow::commitRenderPass(FrameStateSharedPtr frameState)
{
    if (m_windowInited) {
        m_batchManager->renderBatches(frameState);
    }
}

void EGLWindow::updateWindowSensitivity(WindowMask sensitivity)
{
    if (m_windowInited) {
        if (m_winSensitivity != sensitivity) {
            m_winSensitivity = sensitivity;
            auto eglSensitivity = getWindowSensitivity(m_winSensitivity);
            screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_SENSITIVITY, &eglSensitivity);
        }
    }
}
}
