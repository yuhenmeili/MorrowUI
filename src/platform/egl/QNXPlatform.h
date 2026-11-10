//
// Created by 0060328 on 25-9-18.
//

#ifndef QNXPLATFORM_H
#define QNXPLATFORM_H
#include <atomic>
#include <vector>

#include "EGLOperationsQNX.h"
#include "EGLWindow.h"
#include "InputEventsManager.h"
#include "Platform.h"
#include "QNXInputProvider.h"
#include "PresentStage.h"

namespace morrow {
class QNXPlatform : public Platform {
public:
    /// info 为主窗口（UI 管线承载者）；extraWindowInfos 为第二块屏起的窗口
    ///（displayId 须与主窗口不同）。screenMode 见 ScreenRenderMode。
    QNXPlatform(const WindowInfo& info,
                const std::vector<WindowInfo>& extraWindowInfos = {},
                ScreenRenderMode screenMode = ScreenRenderMode::Mirror);

    ~QNXPlatform() override = default;

    void initialize(bool multithread, const RenderDeviceOptions& deviceOptions = {}) override;

    bool beginFrame(FrameStateSharedPtr frameState) override;

    void beginRenderPass(FrameStateSharedPtr frameState) override;

    void updateWidgets(FrameStateSharedPtr frameState) override;

    void commitRenderPass(FrameStateSharedPtr frameState) override;

    void endFrame() override;

    /// 空闲等待：阻塞至新输入唤醒（独立采集线程 notify）或超时（R1 方案 B）
    void waitForEvents(double timeoutSeconds) override;

    /// 唤醒空闲等待（RenderingThread::requestRender 在 worker 任务投递时调用）
    void wakeEventLoop() override;

    void terminate() override;

    InputEventsManagerSharedPtr getInputManager() override;

    /// 多屏镜像：绑定/解绑离屏内容 RT（Engine 在主窗口 UI 管线前后调用）
    void beginContentPass(FrameStateSharedPtr frameState) override;

    void endContentPass(FrameStateSharedPtr frameState) override;

    [[nodiscard]] bool isMirrorActive() const override { return m_mirrorActive; }

    ESContextSharedPtr getContext();

private:
    EGLOperationsQNXSharedPtr m_egl;
    InputEventsManagerSharedPtr m_inputEventsManager;
    std::shared_ptr<QNXInputProvider> m_qnxInputProvider;
    std::shared_ptr<PresentStage> m_presentStage;
    bool m_mirrorActive = false;
    /**
    * 休眠恢复之后抛弃事件的帧数，暂时没有完美解决方案，多线程方案上下文移除的时候会异常
    */
    std::atomic_int m_skipEventFrameCount{0};
};

using QnxPlatformSharedPtr = std::shared_ptr<QNXPlatform>;
} // morrow

#endif //QNXPLATFORM_H
