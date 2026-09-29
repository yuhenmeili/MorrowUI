//
// Created by lance on 2023/3/22.
//

#include "morrow/scene3d/FrameState.h"

#include "morrow/BatchStatistics.h"
#include "FrameStateLegacy.h"

namespace morrow
{
FrameState::FrameState()
    : scene3DLegacy(std::make_unique<Scene3DLegacy>()),
      batchStatistics(std::make_shared<BatchStatistics>()) {}
FrameState::~FrameState() = default;

FrameState& FrameState::operator=(const FrameState& other) {
    if (this == &other) {
        return *this;
    }
    frameNumber = other.frameNumber;
    screenAlpha = other.screenAlpha;
    deltaTime = other.deltaTime;
    camera = other.camera;
    inputEventsManager = other.inputEventsManager;
    callAfterRender = other.callAfterRender;
    callAfterTouched = other.callAfterTouched;
    debugWidgetsAfterAnimate = other.debugWidgetsAfterAnimate;
    batchManager = other.batchManager;
    ssboManager = other.ssboManager;
    currentClip = other.currentClip;
    clipStack = other.clipStack;
    framebufferWidth = other.framebufferWidth;
    framebufferHeight = other.framebufferHeight;
    clearColor = other.clearColor;
    drawCallCount = other.drawCallCount;
    fps = other.fps;
    isSSBOSupport = other.isSSBOSupport;
    batchStatistics = other.batchStatistics;
    perspectiveCamera = other.perspectiveCamera;
    scene3DPassContext = other.scene3DPassContext;
    return *this;
}
} // namespace morrow
