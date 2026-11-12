//
// Created by 0060328 on 25-9-18.
//

#ifndef PLATFORM_H
#define PLATFORM_H
#include <memory>
#include <unordered_map>
#include <vector>

#include "morrow/scene3d/FrameState.h"
#include "InputEventsManager.h"
#include "morrow/ClipRect.h"
#include "morrow/RenderDeviceOptions.h"

namespace morrow {
struct WindowInfo;
using WindowInfoSharedPtr = std::shared_ptr<WindowInfo>;

class PlatformWindow;
using PlatformWindowSharedPtr = std::shared_ptr<PlatformWindow>;

class Platform : public std::enable_shared_from_this<Platform> {
public:
    virtual ~Platform() = default;

    /// multithread 选择单/多线程渲染；deviceOptions 携带渲染设备启动配置
    /// （CommandBuffer 容量、shader 二进制缓存、SSBO 预设）。
    virtual void initialize(bool multithread, const RenderDeviceOptions& deviceOptions = {});

    /// ────────── 输入阶段 ──────────
    /// poll 输入事件 + 解析 hit-test 目标
    virtual bool beginFrame(FrameStateSharedPtr frameState) = 0;

    /// 按需渲染空闲等待。平台可使用原生事件等待，默认使用短超时 sleep。
    virtual void waitForEvents(double timeoutSeconds);

    /// 从其他线程唤醒事件等待。默认实现为空。
    virtual void wakeEventLoop();

    /// 将输入事件派发到目标 Widget（原 eventHandler，提升为 public）
    void dispatchEvents(const FrameStateSharedPtr& frameState);

    /// ────────── 渲染阶段 ──────────
    /// GPU 准备：setViewport、clear、绑定 framebuffer
    virtual void beginRenderPass(FrameStateSharedPtr frameState) = 0;

    /// Widget 树递归更新（纯 CPU，不涉及 GPU）
    virtual void updateWidgets(FrameStateSharedPtr frameState);

    /// Widget 树 lateUpdate（在所有 standard update 完成后）
    virtual void lateUpdateWidgets(FrameStateSharedPtr frameState);

    /// GPU 提交：渲染合批 + 提交 draw call
    virtual void commitRenderPass(FrameStateSharedPtr frameState) = 0;

    /// ────────── 交换阶段 ──────────
    /// swap buffers / present
    virtual void endFrame() = 0;

    virtual void terminate() = 0;

    virtual InputEventsManagerSharedPtr getInputManager() = 0;

    /// 多屏镜像：渲染前绑定离屏内容 RT（Mirror 模式下由 Engine 在 UI 管线
    /// 前后调用）；基类空实现。
    virtual void beginContentPass(FrameStateSharedPtr frameState) {
    }

    /// 多屏镜像：解绑内容 RT（内含 MSAA resolve）
    virtual void endContentPass(FrameStateSharedPtr frameState) {
    }

    /// 镜像模式是否激活（Mirror 且配置了多块屏）；激活时 endFrame 由
    /// PresentStage 负责各屏贴图 + present
    [[nodiscard]] virtual bool isMirrorActive() const { return false; }

    [[nodiscard]] PlatformWindowSharedPtr getWindow() const;

    /// 全部窗口（第一个即 getWindow()）。Mirror 模式下仅第一个窗口承载 UI
    /// 管线；Independent 模式下每个窗口各自渲染各自的 Scene2D 子树
    [[nodiscard]] const std::vector<PlatformWindowSharedPtr>& getWindows() const;

    /// 由平台子类在创建窗口后调用；m_window 恒指向第一个窗口
    void setWindows(std::vector<PlatformWindowSharedPtr> windows);

    /// 单窗口平台（如 WGL/GLFW）使用：等价于 setWindows({window})
    void setWindow(PlatformWindowSharedPtr window);

    [[nodiscard]] bool shouldClose() const;

    [[nodiscard]] int32_t getRequestedSamples() const {
        return m_requestedSamples;
    }

protected:
    std::shared_ptr<Widget> findTopmostInteractiveWidget(const std::shared_ptr<Widget>& root, float x, float y);

    std::shared_ptr<Widget> findTopmostInteractiveWidget(
        const std::shared_ptr<Widget>& root,
        float x,
        float y,
        const ClipRect& inheritedClip);

    void resolveInputTargets(const FrameStateSharedPtr& frameState);

    void ensureRenderCapabilitiesInitialized();

    PlatformWindowSharedPtr m_window;
    /// 全部窗口，m_window 恒等于 m_windows 的第一个；单窗口场景只有一个元素
    std::vector<PlatformWindowSharedPtr> m_windows;
    int32_t m_requestedSamples = 1;
    bool m_isSSBOSupport = false;
    /// SSBO 能力预设（RenderDeviceOptions::ssboSupportPreset），见 ensureRenderCapabilitiesInitialized。
    int32_t m_ssboSupportPreset = -1;
    /// 按压捕获表。key = (windowIndex << 32) | touchID：多屏下两块屏的
    /// 触摸各自从 id=0 开始编号，裸 touchID 会跨屏冲突
    std::unordered_map<int64_t, std::weak_ptr<Widget>> m_pointerCaptureTargets;
    std::weak_ptr<Widget> m_hoverTarget;
    std::weak_ptr<Widget> m_keyboardFocusTarget;
};

using PlatformSharedPtr = std::shared_ptr<Platform>;
}  // namespace morrow

#endif  // PLATFORM_H
