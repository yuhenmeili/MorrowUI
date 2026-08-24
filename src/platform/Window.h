//
// Created by 0060328 on 25-9-18.
//

#ifndef WINDOW_H
#define WINDOW_H
#include <memory>
#include <string>

#include "base/UIWidget.h"
#include "core/Observable.h"

namespace morrow {
enum class CursorShape {
    Arrow,
    IBeam,
    Hand,
    ResizeHorizontal,
    ResizeVertical,
    ResizeAll,
    Forbidden,
};

struct WindowEvents {
    Observable<const Vector2&> onWindowSizeChanged;
    Observable<const Vector2&> onFramebufferSizeChanged;
    Observable<float> onContentScaleChanged;
};

enum WindowMask : int32_t {
    SCREEN_SENSITIVITY_MASK_ALWAYS = (1 << 0),
    SCREEN_SENSITIVITY_MASK_NEVER = (2 << 0),
    SCREEN_SENSITIVITY_MASK_NO_FOCUS = (1 << 3),
    SCREEN_SENSITIVITY_MASK_FULLSCREEN = (1 << 4),
    SCREEN_SENSITIVITY_MASK_CONTINUE = (1 << 5),
    SCREEN_SENSITIVITY_MASK_STOP = (2 << 5),
    SCREEN_SENSITIVITY_MASK_POINTER_BRUSH = (1 << 7),
    SCREEN_SENSITIVITY_MASK_FINGER_BRUSH = (1 << 8),
    SCREEN_SENSITIVITY_MASK_STYLUS_BRUSH = (1 << 9),
    SCREEN_SENSITIVITY_MASK_OVERDRIVE = (1 << 10),
    SCREEN_SENSITIVITY_MASK_CLIPPED = (1 << 11),
};

struct WindowInfo
{
    virtual ~WindowInfo() = default;
    std::string name = "default";
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 1920;
    int32_t height = 1080;
    int32_t samples = 1;
    int32_t alpha = 1.0f;
    //egl
    int32_t displayId = 3;
    int32_t zorder = 4000;
    WindowMask sensitivity = WindowMask::SCREEN_SENSITIVITY_MASK_ALWAYS;
};

using WindowInfoSharedPtr = std::shared_ptr<WindowInfo>;

class Window : public UIWidget{
public:
    Window();

    ~Window() override = default;

    virtual bool initializeIfNeeded();

    virtual bool isWindowShouldClose();

    virtual void setClearColor(float r, float g, float b, float a);

    virtual void setCursorShape(CursorShape shape);

    WindowEvents& events();

    /// ────────── 渲染阶段（替代原 update()）──────────
    /// GPU 准备：viewport、clear、framebuffer
    virtual void beginRenderPass(FrameStateSharedPtr frameState);
    /// Widget 树递归更新（纯 CPU）
    virtual void updateWidgets(FrameStateSharedPtr frameState);
    /// Widget 树 lateUpdate（在所有 standard update 完成后）
    virtual void lateUpdateWidgets(FrameStateSharedPtr frameState);
    /// GPU 提交：合批渲染
    virtual void commitRenderPass(FrameStateSharedPtr frameState);

    virtual void terminate();

    virtual void* getSurface() const;
protected:
    std::shared_ptr<BatchManager> m_batchManager;
    WindowEvents m_events;
};

using WindowSharedPtr = std::shared_ptr<Window>;
}

#endif //WINDOW_H
