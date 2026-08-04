//
// Created by lance on 2023/12/15.
//

#include "RenderingThread.h"

namespace morrow
{
void RenderingThread::run(PlatformSharedPtr platform, bool multithread)
{
    m_platform = platform;
    m_esDevice = std::make_shared<RenderDeviceProxy>(platform, false);
    m_esDevice->run(multithread);
}

const ThreadBufferESDeviceSharedPtr& RenderingThread::getDevice() const
{
    return m_esDevice;
}

void RenderingThread::requestRender()
{
    const bool wasRequested = m_needRender.exchange(true, std::memory_order_acq_rel);
    if (!wasRequested) {
        if (const auto platform = m_platform.lock()) {
            platform->wakeEventLoop();
        }
    }
}

bool RenderingThread::isNeedRender() const
{
    return m_needRender.load(std::memory_order_acquire);
}

bool RenderingThread::consumeRenderRequest()
{
    return m_needRender.exchange(false, std::memory_order_acq_rel);
}

void RenderingThread::resetRenderStatus()
{
    m_needRender.store(false, std::memory_order_release);
}
} // MORROWGUI
