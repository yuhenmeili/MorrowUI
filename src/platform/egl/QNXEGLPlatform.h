//
// Created by 0060328 on 25-9-18.
//

#ifndef QNXPLATFORM_H
#define QNXPLATFORM_H
#include <atomic>

#include "EGLOperationsQNX.h"
#include "EGLWindow.h"
#include "../InputEventsManager.h"
#include "../Platform.h"
#include "QNXInputProvider.h"

namespace morrow {
class QNXEGLPlatform : public Platform {
public:
    QNXEGLPlatform(const WindowInfo& info);

    ~QNXEGLPlatform() override = default;

    void initialize(bool multithread) override;

    bool beginFrame(FrameStateSharedPtr frameState) override;

    void update(FrameStateSharedPtr frameState) override;

    void endFrame() override;

    void terminate() override;

    InputEventsManagerSharedPtr getInputManager() override;

    ESContextSharedPtr getContext();

private:
    EGLOperationsQNXSharedPtr m_egl;
    InputEventsManagerSharedPtr m_inputEventsManager;
    std::shared_ptr<QNXInputProvider> m_qnxInputProvider;
    /**
    * 休眠恢复之后抛弃事件的帧数，暂时没有完美解决方案，多线程方案上下文移除的时候会异常
    */
    std::atomic_int m_skipEventFrameCount{0};
};

using QnxPlatformSharedPtr = std::shared_ptr<QNXEGLPlatform>;
} // morrow

#endif //QNXPLATFORM_H

