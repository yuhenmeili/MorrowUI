//
// Created by 0060328 on 25-9-18.
//
// Window — 引擎内部平台窗口（不再对外暴露，见 §6.7）。
// 只负责 surface / swap / 输入泵 / 清屏色 / 剪贴板等窗口职责，并持有
// 2D UI 根（Scene2D）；UI 树经 Scene2D 对外（Engine::getScene2D）。
//

#ifndef WINDOW_H
#define WINDOW_H
#include <memory>
#include <string>

#include "morrow/WindowInfo.h"
#include "morrow/base/Scene2D.h"
#include "morrow/base/TouchEvent.h"
#include "morrow/core/Observable.h"
#include "morrow/scene3d/FrameState.h"

namespace morrow {

struct WindowEvents {
    Observable<const Vector2&> onWindowSizeChanged;
    Observable<const Vector2&> onFramebufferSizeChanged;
    Observable<float> onContentScaleChanged;
    /// 原始键盘/字符输入（KEY_DOWN / CHARACTER），在 Widget 派发前广播；
    /// 由 Engine 转播到 EngineEvents::onRawKeyboardInput。
    Observable<const TouchEvent&> onRawKeyboardInput;
};

class BatchManager;

class Window {
public:
    Window();

    virtual ~Window() = default;

    virtual bool initializeIfNeeded();

    virtual bool isWindowShouldClose();

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

    /// 渲染请求（原 UIWidget::requestRender；Window 内部回调使用）。
    void requestRender(const char* reason = nullptr);

protected:
    std::shared_ptr<BatchManager> m_batchManager;
    std::shared_ptr<Scene2D> m_scene2D;
    WindowEvents m_events;
};

using WindowSharedPtr = std::shared_ptr<Window>;
}

#endif //WINDOW_H
