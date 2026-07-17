//
// Created by lance on 2023/12/15.
//

#ifndef MORROW_RENDERER_RENDERINGTHREAD_H_
#define MORROW_RENDERER_RENDERINGTHREAD_H_

#include "Platform.h"
#include "RenderDeviceProxy.h"

namespace morrow
{

class RenderingThread
{
public:
    void run(PlatformSharedPtr platform, bool multithread = false);

    const ThreadBufferESDeviceSharedPtr& getDevice() const;

    void requestRender();

    bool isNeedRender() const;

    void resetRenderStatus();

private:
    ThreadBufferESDeviceSharedPtr m_esDevice;
    bool m_needRender = true;
};

using RenderingThreadSharedPtr = std::shared_ptr<RenderingThread>;

} // MORROWGUI

#endif //MORROW_RENDERER_RENDERINGTHREAD_H_
