//
// Created by lance on 2023/12/8.
// Refactored: removed old XXXHandle wrapper classes; GPU resources now use integer ResourceHandle.
//

#ifndef MORROW_RENDERER_THREADESDEVICEBASE_H_
#define MORROW_RENDERER_THREADESDEVICEBASE_H_

#include "RenderDevice.h"
#include <thread>
#include "PlatformSemaphore.h"
#include "GLRenderDevice.h"
#include "Platform.h"

namespace morrow {
class RenderDeviceProxyBase;

// ---------------------------------------------------------------------------
// RenderDeviceProxyBase — 渲染线程代理基类
//
// 职责：
//   - 管理渲染线程生命周期
//   - 提供 CommandBuffer 编码/执行框架
//   - 单线程直通 / 多线程命令队列双模式
//
// 资源管理变更：
//   旧 XXXHandle 包装类（GPUProgramHandle / Texture2DHandle 等）已移除。
//   前端直接持有 HwXXX 值类型 Handle，通过 RenderDevice 接口提交命令。
//   渲染线程通过 ResourceRegistry 解析 Handle 到 GL 对象。
// ---------------------------------------------------------------------------

class RenderDeviceProxyBase : public RenderDevice {
public:
    enum WaitType {
        WaitType_Common,
        WaitType_OnwerShip,
        WaitType_Present,
        WaitType_Max
    };

protected:
    bool m_returnResImmediately;
    bool m_threaded;
    bool m_isInPresenting;
    GLRenderDevicePtr m_realDevice;

private:
    std::shared_ptr<std::thread> m_thread;
    bool m_quit;
    Semaphore m_waitSem[WaitType_Max];

public:
    RenderDeviceProxyBase(PlatformSharedPtr platform, bool returnResImmediately);

    ~RenderDeviceProxyBase() override;

    /** Called by destructor before join; override to unblock render thread (e.g. double-buffer semaphore). */
    virtual void signalThreadToExit() {
    }

    bool isCreateResInBlockMode() const;

    void waitForSignal(WaitType waitType = WaitType_Common);

    void signal(WaitType waitType = WaitType_Common);

    void waitForPresent();

    void signalPresent();

    void run(bool multithread);

protected:
    void stopRenderThread();

    /** Override in derived class for double-buffered command queue (e.g. wait frame then drain). */
    virtual void runCommand();

    bool isQuit() const { return m_quit; }
};
} // MORROWGUI

#endif //MORROW_RENDERER_THREADESDEVICEBASE_H_
