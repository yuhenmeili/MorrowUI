//
// Created by 0060328 on 25-9-18.
//

#ifndef WINDOWPLATFORM_H
#define WINDOWPLATFORM_H
#include "Platform.h"
#include <memory>

namespace morrow {

class WGLInputProvider;

class WGLPlatform : public Platform {
public:
    WGLPlatform(const WindowInfo& info);

    ~WGLPlatform() override = default;

    void initialize(bool multithread) override;

    bool beginFrame(FrameStateSharedPtr frameState) override;

    void update(FrameStateSharedPtr frameState) override;

    void endFrame() override;

    void terminate() override;

    InputEventsManagerSharedPtr getInputManager() override;

private:
    InputEventsManagerSharedPtr m_inputManager;
    std::shared_ptr<WGLInputProvider> m_wglInputProvider;
};

using WGLPlatformSharedPtr = std::shared_ptr<WGLPlatform>;
} // morrow

#endif //WINDOWPLATFORM_H
