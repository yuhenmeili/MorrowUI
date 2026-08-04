//
// Created by lance on 2023/12/15.
//

#ifndef MORROW_RENDERER_RENDERINGTHREAD_H_
#define MORROW_RENDERER_RENDERINGTHREAD_H_

#include <atomic>

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

    bool consumeRenderRequest();

    void resetRenderStatus();

private:
    ThreadBufferESDeviceSharedPtr m_esDevice;
    std::weak_ptr<Platform> m_platform;
    std::atomic_bool m_needRender{true};
};

using RenderingThreadSharedPtr = std::shared_ptr<RenderingThread>;

} // MORROWGUI

#endif //MORROW_RENDERER_RENDERINGTHREAD_H_
