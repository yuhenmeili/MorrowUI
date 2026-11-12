//
// Created by 0060328 on 25-9-18.
//
// PlatformWindow — 引擎内部平台窗口实现（不出公共接口，§6.7）。
// 窗口职责的公共面在 include/morrow/Window.h 门面（经 Engine::getWindow
// 持有）；本类只负责 surface / swap / 输入泵 / 清屏色 / 剪贴板等平台
// 职责，并持有 2D UI 根（Scene2D）；渲染 pass 与生命周期由 Engine
// 渲染循环驱动。
//

#ifndef PLATFORMWINDOW_H
#define PLATFORMWINDOW_H
#include <memory>
#include <string>

#include "morrow/WindowEvents.h"
#include "morrow/WindowInfo.h"
#include "morrow/base/Scene2D.h"
#include "morrow/base/TouchEvent.h"
#include "morrow/core/Observable.h"
#include "morrow/scene3d/FrameState.h"

namespace morrow {
class BatchManager;

class PlatformWindow {
public:
    PlatformWindow();

    virtual ~PlatformWindow() = default;

    virtual bool initializeIfNeeded();

    virtual bool isWindowShouldClose();

    /// 窗口（screen window / EGL surface）是否已初始化成功，
    /// 多屏 present / 渲染时跳过初始化失败的窗口
    virtual bool isWindowInited() const {
        return true;
    }

    /// 窗口逻辑位置（多屏输入坐标换算与渲染 pass 相机使用同一套参数）
    virtual Vector2 getWindowPosition() const {
        return {0.0f, 0.0f};
    }

    /// 窗口逻辑尺寸（多屏输入命中与共享相机重配用；EGL 即窗口创建尺寸）
    virtual Vector2 getWindowSize() const {
        return {0.0f, 0.0f};
    }

    /// 窗口创建时绑定的物理屏 displayId（与 WindowInfo.displayId 一致；
    /// 多屏按屏查找/显隐接口用）
    virtual int32_t getDisplayId() const {
        return 0;
    }

    /// 窗口显隐（QNX 走 Screen 合成器 SCREEN_PROPERTY_VISIBLE：隐藏期间
    /// 该窗口的渲染 pass 与 present 一并跳过，重新显示恢复原状）。
    /// 桌面平台默认空实现。
    virtual void setVisible(bool /*visible*/) {
    }

    /// 窗口在合成器侧是否可见
    virtual bool isScreenVisible() const {
        return true;
    }

    /// 原生窗口 surface（平台私有实现层专用，不出公共接口）：
    /// EGL 平台为 EGLSurface，GLFW 平台为 GLFWwindow*；镜像上屏
    /// （PresentStage）据此逐窗口 makeCurrent / present。
    virtual void* nativeSurface() const {
        return nullptr;
    }

    virtual void setClearColor(float r, float g, float b, float a);

    virtual void setCursorShape(CursorShape shape);

    WindowEvents& events();

    /// 2D UI 根（引擎内部挂载点：输入命中、调试覆盖层、编辑器壳）。
    [[nodiscard]] std::shared_ptr<Scene2D> scene2D() const;

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

    /// framebuffer 尺寸（像素）。窗口未初始化时返回 {0, 0}。
    virtual Vector2 framebufferSize() const;

    /// 系统剪贴板读写。平台不支持时写入为空操作、读取返回空串。
    virtual void setClipboardText(const std::string& text);

    virtual std::string clipboardText() const;

    /// 渲染请求（原 UIWidget::requestRender；PlatformWindow 内部回调使用）。
    void requestRender(const char* reason = nullptr);

protected:
    std::shared_ptr<BatchManager> m_batchManager;
    std::shared_ptr<Scene2D> m_scene2D;
    WindowEvents m_events;
};

using PlatformWindowSharedPtr = std::shared_ptr<PlatformWindow>;
}

#endif //PLATFORMWINDOW_H