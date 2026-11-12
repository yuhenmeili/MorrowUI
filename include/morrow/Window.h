//
// Window — 窗口公共门面（多屏模型，§6.7 修订）。
//
// 多屏后 Engine 不再是单窗口 facade：窗口相关能力（清屏色 / 剪贴板 /
// 光标形态 / framebuffer 查询 / 各窗口事件 / Scene2D 根）归属本门面，
// 应用经 Engine::getWindow(index) 持有（0 = 主窗口）。
// 引擎内部职责——渲染 pass（beginRenderPass 等）、生命周期（terminate）、
// 原生 surface——仍留在内部实现类（PlatformWindow），不出公共接口。
//

#ifndef MORROW_WINDOW_H
#define MORROW_WINDOW_H

#include <memory>
#include <string>

#include "morrow/WindowEvents.h"
#include "morrow/WindowInfo.h"
#include "morrow/base/Scene2D.h"

namespace morrow
{

class PlatformWindow;

class Window
{
public:
    ~Window();

    /// 窗口清屏色。Mirror 模式下副屏物理上屏为 letterbox 贴图（黑边硬
    /// 编码），独立清屏色仅 Independent / 单窗口模式生效。
    void setClearColor(float r, float g, float b, float a);

    /// 鼠标指针形态（QNX/EGL 平台暂无实现，空操作）。
    void setCursorShape(CursorShape shape);

    /// 系统剪贴板读写（QNX/EGL 平台暂未接入：写为空操作、读返回空串）。
    void setClipboardText(const std::string& text);

    [[nodiscard]] std::string clipboardText() const;

    /// framebuffer 尺寸（像素）。窗口未就绪时返回 {0, 0}。
    [[nodiscard]] Vector2 framebufferSize() const;

    /// 本窗口的 2D UI 根节点（向其 addChild 搭建各屏界面）。
    [[nodiscard]] std::shared_ptr<Scene2D> scene2D() const;

    /// 本窗口事件（尺寸 / framebuffer / 内容缩放 / 原始键盘输入）。
    /// Engine::events() 仅转播主窗口（index 0）的原始键盘与 framebuffer 事件，
    /// 其他窗口经此订阅。
    [[nodiscard]] WindowEvents& events();

    /// 唤醒一帧渲染（EngineOptions::enableRequestRender = true 的按需
    /// 渲染模式下使用）。
    void requestRender(const char* reason = nullptr);

private:
    friend class Engine;

    explicit Window(std::shared_ptr<PlatformWindow> impl);

    std::shared_ptr<PlatformWindow> m_impl;
};

using WindowSharedPtr = std::shared_ptr<Window>;

} // namespace morrow

#endif // MORROW_WINDOW_H
