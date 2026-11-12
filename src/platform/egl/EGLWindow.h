//
// Created by lance on 2023/7/18.
//

#ifndef MORROW_EGL_WINDOW_H_
#define MORROW_EGL_WINDOW_H_

#include <memory>
#include <string>
#include <vector>

#include "EGLOperationsQNX.h"
#include "PlatformWindow.h"
#include <screen/screen.h>

namespace morrow
{
class EGLWindow;

using EGLWindowSharedPtr = std::shared_ptr<EGLWindow>;
class EGLWindow : public PlatformWindow
{
public:
    static EGLWindowSharedPtr create(EGLOperationsQNXSharedPtr context, const WindowInfo& info);

    ~EGLWindow() override;

    bool initializeIfNeeded() override;

    bool isWindowInited() const override { return m_windowInited; }

    Vector2 getWindowPosition() const override { return m_windowPosition; }

    Vector2 getWindowSize() const override { return m_windowSize; }

    /// screen window 句柄（触摸事件 SCREEN_PROPERTY_WINDOW 反查归属用）
    screen_window_t getScreenWindow() const { return screen_win; }

    void beginRenderPass(FrameStateSharedPtr frameState) override;
    void updateWidgets(FrameStateSharedPtr frameState) override;
    void commitRenderPass(FrameStateSharedPtr frameState) override;

    void updateWindowSensitivity(WindowMask sensitivity);

    void setClearColor(float r, float g, float b, float a) override;

    void setScreenAlpha(float alpha = 1.0f);

    Vector2 framebufferSize() const override { return m_windowSize; }

    // QNX screen 剪贴板暂未接入：写入为空操作，读取返回空串
    void setClipboardText(const std::string& /*text*/) override {}

    std::string clipboardText() const override { return {}; }

    /// 实现层专用：EGLSurface，不出公共接口（PlatformWindow::nativeSurface 虚接口）
    void* nativeSurface() const override { return m_eglSurface; }

private:
    EGLWindow(EGLOperationsQNXSharedPtr context, const WindowInfo& info);

    void initialize();

    int32_t initWindow();

    int32_t initDisplay();

    int32_t initSurface();

    void terminate() override;

    int32_t static getWindowSensitivity(WindowMask mask);

private:
    screen_window_t screen_win = nullptr;
    screen_display_t* screen_display_array = nullptr;
    EGLSurface m_eglSurface = EGL_NO_SURFACE;

    //QNX Display
    EGLOperationsQNXSharedPtr m_platform;
    std::string m_widgetName;  // WindowInfo::name，初始化失败日志用
    int32_t m_displayId = 0;
    int32_t m_zorder = 0;
    Vector2 m_windowPosition = {0.0f, 0.0f};
    Vector2 m_windowSize = {0.0f, 0.0f};
    int32_t m_windowAlpha = 255;
    int32_t m_windowAlphaMode = SCREEN_PRE_MULTIPLIED_ALPHA;
    WindowMask m_winSensitivity = WindowMask::SCREEN_SENSITIVITY_MASK_ALWAYS;
    bool m_windowInited = false;
    int32_t m_initAttemptCounts = 0;

    Vector4 m_clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
    std::atomic<float> m_screenAlpha{1.0f};
};
}

#endif //MORROW_EGL_WINDOW_H_
