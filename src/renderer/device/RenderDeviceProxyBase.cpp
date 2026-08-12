//
// Created by lance on 2023/12/8.
// Refactored: old XXXHandle class implementations removed.
//

#include <functional>
#include "RenderDeviceProxyBase.h"

namespace morrow {

//-------------------------------------------------------------------------------------------------------------

RenderDeviceProxyBase::RenderDeviceProxyBase(PlatformSharedPtr platform, bool returnResImmediately)
    : m_returnResImmediately(returnResImmediately), m_threaded(false), m_isInPresenting(false), m_quit(false) {
    m_realDevice = std::make_shared<GLRenderDevice>(platform);
}

RenderDeviceProxyBase::~RenderDeviceProxyBase() {
    stopRenderThread();
}

void RenderDeviceProxyBase::stopRenderThread() {
    if (!m_thread)
        return;
    m_quit = true;
    signalThreadToExit();
    m_thread->join();
    m_thread.reset();
}

bool RenderDeviceProxyBase::isCreateResInBlockMode() const {
    return m_returnResImmediately;
}

void RenderDeviceProxyBase::waitForSignal(WaitType waitType) {
    m_waitSem[waitType].waitForSignal();
}

void RenderDeviceProxyBase::signal(WaitType waitType) {
    m_waitSem[waitType].signal();
}

void RenderDeviceProxyBase::waitForPresent() {
    waitForSignal(WaitType_Present);
}

void RenderDeviceProxyBase::signalPresent() {
    signal(WaitType_Present);
    m_isInPresenting = false;
}

void RenderDeviceProxyBase::runCommand() {
}

void RenderDeviceProxyBase::run(bool multithread) {
    m_threaded = multithread;
    if (m_threaded) {
        m_thread = std::make_shared<std::thread>(std::bind(&RenderDeviceProxyBase::runCommand, this));
    }
}
} // MORROWGUI
