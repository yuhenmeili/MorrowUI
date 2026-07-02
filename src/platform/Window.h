//
// Created by 0060328 on 25-9-18.
//

#ifndef WINDOW_H
#define WINDOW_H
#include <memory>
#include <string>

#include "base/UIWidget.h"

namespace morrow {
enum WindowMask : int32_t
{
    DEFAULT_MASK = 0,
    CONTINUE_MASK = 1,
    ALWAYS_MASK = 2,
    NEVER_MASK = 3
};

struct WindowInfo
{
    virtual ~WindowInfo() = default;
    std::string name = "default";
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 1280;
    int32_t height = 960;
    int32_t samples = 1;
    int32_t alpha = 1.0f;
    //egl
    int32_t displayId = 3;
    int32_t zorder = 4000;
    WindowMask sensitivity = WindowMask::DEFAULT_MASK;
};

using WindowInfoSharedPtr = std::shared_ptr<WindowInfo>;

class Window : public UIWidget{
public:
    Window();

    ~Window() override = default;

    virtual bool initializeIfNeeded();

    virtual bool isWindowShouldClose();

    virtual void setClearColor(float r, float g, float b, float a);

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
};

using WindowSharedPtr = std::shared_ptr<Window>;
}

#endif //WINDOW_H
