//
// Created by 0060328 on 25-9-18.
//

#ifndef PLATFORM_H
#define PLATFORM_H
#include <memory>
#include <unordered_map>

#include "FrameState.h"
#include "InputEventsManager.h"

namespace morrow {
struct WindowInfo;
using WindowInfoSharedPtr = std::shared_ptr<WindowInfo>;

class Window;
using WindowSharedPtr = std::shared_ptr<Window>;

class Platform : public std::enable_shared_from_this<Platform>{
public:
    virtual ~Platform() = default;

    virtual void initialize(bool multithread);

    /// ────────── 输入阶段 ──────────
    /// poll 输入事件 + 解析 hit-test 目标
    virtual bool beginFrame(FrameStateSharedPtr frameState) = 0;

    /// 将输入事件派发到目标 Widget（原 eventHandler，提升为 public）
    void dispatchEvents(const FrameStateSharedPtr& frameState);

    /// ────────── 渲染阶段 ──────────
    /// GPU 准备：setViewport、clear、绑定 framebuffer
    virtual void beginRenderPass(FrameStateSharedPtr frameState) = 0;

    /// Widget 树递归更新（纯 CPU，不涉及 GPU）
    virtual void updateWidgets(FrameStateSharedPtr frameState);

    /// GPU 提交：渲染合批 + 提交 draw call
    virtual void commitRenderPass(FrameStateSharedPtr frameState) = 0;

    /// ────────── 交换阶段 ──────────
    /// swap buffers / present
    virtual void endFrame() = 0;

    virtual void terminate() = 0;

    virtual InputEventsManagerSharedPtr getInputManager() = 0;

    [[nodiscard]] WindowSharedPtr getWindow() const;

    [[nodiscard]] bool shouldClose() const;

    [[nodiscard]] int32_t getRequestedSamples() const { return m_requestedSamples; }

protected:
    std::shared_ptr<Widget> findTopmostInteractiveWidget(const std::shared_ptr<Widget>& root, float x, float y);

    void resolveInputTargets(const FrameStateSharedPtr& frameState);

    void ensureRenderCapabilitiesInitialized();

    WindowSharedPtr m_window;
    int32_t m_requestedSamples = 1;
    bool m_isSSBOSupport = false;
    std::unordered_map<int32_t, std::weak_ptr<Widget>> m_pointerCaptureTargets;
};

using PlatformSharedPtr = std::shared_ptr<Platform>;
} // morrow

#endif //PLATFORM_H
