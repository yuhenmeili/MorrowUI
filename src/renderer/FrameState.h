//
// Created by lance on 2023/3/22.
//

#ifndef MORROW_FRAMESTATE_H
#define MORROW_FRAMESTATE_H

#include <cstdint>
#include <functional>
#include <vector>
#include <memory>

#include "Scene3DPassContext.h"
#include "Vector3.h"

namespace morrow {
class PerspectiveCamera;
class Widget;
class InputEventsManager;
class OrthographicCamera;
class BatchManager;
class SSBOManager;
class UBO;

struct FrameState {
    uint64_t frameNumber = 0;
    float screenAlpha = 1.0f;
    double deltaTime = 0.0;
    std::shared_ptr<OrthographicCamera> camera;
    std::shared_ptr<InputEventsManager> inputEventsManager;
    std::vector<std::function<void()>> callAfterRender{};
    std::vector<std::function<void()>> callAfterTouched{};
    /**
     * 动效完成之后打印一次组件和孩子状态
     */
    std::vector<std::shared_ptr<Widget>> debugWidgetsAfterAnimate{};
    std::shared_ptr<BatchManager> batchManager;
    std::shared_ptr<SSBOManager> ssboManager;

    uint32_t drawCallCount = 0;
    uint32_t fps = 0;
    bool isSSBOSupport = false;

    std::shared_ptr<PerspectiveCamera> perspectiveCamera;  // null=2D path
    Scene3DPassContextSharedPtr scene3DPassContext;
    // Legacy mirrors kept temporarily while 3D code migrates to scene3DPassContext.
    Scene3DLightingState scene3DLighting;
    Scene3DIBLState scene3DIBL;
    UBO* scene3DFrameUBO = nullptr;
};

using FrameStateSharedPtr = std::shared_ptr<FrameState>;
}

#endif //MORROW_FRAMESTATE_H
