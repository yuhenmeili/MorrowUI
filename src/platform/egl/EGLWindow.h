//
// Created by lance on 2023/7/18.
//

#ifndef MORROW_EGL_WINDOW_H_
#define MORROW_EGL_WINDOW_H_

#include <memory>
#include <vector>

#include "EGLOperationsQNX.h"
#include "../Window.h"
#include <screen/screen.h>

namespace morrow
{
class EGLWindow;

using EGLWindowSharedPtr = std::shared_ptr<EGLWindow>;
class EGLWindow : public Window
{
public:
    static EGLWindowSharedPtr create(EGLOperationsQNXSharedPtr context, const WindowInfo& info);

    ~EGLWindow() override;

    bool initializeIfNeeded() override;

    void update(FrameStateSharedPtr frameState) override;

    void updateWindowSensitivity(WindowMask sensitivity);

    void setClearColor(float r, float g, float b, float a) override;

    void setScreenAlpha(float alpha = 1.0f);

    void* getSurface() const override { return m_eglSurface; }

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
    int32_t m_displayId = 0;
    int32_t m_zorder = 0;
    Vector2 m_windowPosition = {0.0f, 0.0f};
    Vector2 m_windowSize = {0.0f, 0.0f};
    int32_t m_windowAlpha = 255;
    int32_t m_windowAlphaMode = SCREEN_PRE_MULTIPLIED_ALPHA;
    WindowMask m_winSensitivity = WindowMask::DEFAULT_MASK;
    bool m_windowInited = false;
    int32_t m_initAttemptCounts = 0;

    Vector4 m_clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
    std::atomic<float> m_screenAlpha{1.0f};
};
}

#endif //MORROW_EGL_WINDOW_H_
