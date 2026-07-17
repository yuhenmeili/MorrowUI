//
// Created by lance on 2023/12/15.
//

#include "RenderingThread.h"

namespace morrow
{
void RenderingThread::run(PlatformSharedPtr platform, bool multithread)
{
    m_esDevice = std::make_shared<RenderDeviceProxy>(platform, false);
    m_esDevice->run(multithread);
}

const ThreadBufferESDeviceSharedPtr& RenderingThread::getDevice() const
{
    return m_esDevice;
}

void RenderingThread::requestRender()
{
    m_needRender = true;
}

bool RenderingThread::isNeedRender() const
{
    return m_needRender;
}

void RenderingThread::resetRenderStatus()
{
    m_needRender = false;
}
} // MORROWGUI