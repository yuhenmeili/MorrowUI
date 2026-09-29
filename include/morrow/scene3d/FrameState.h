//
// Created by lance on 2023/3/22.
//

#ifndef MORROW_FRAMESTATE_H
#define MORROW_FRAMESTATE_H

#include <cstdint>
#include <functional>
#include <vector>
#include <memory>

#include "morrow/ClipRect.h"
#include "Vector3.h"
#include "Vector4.h"

namespace morrow {
using namespace Math;
class PerspectiveCamera;
class Widget;
class InputEventsManager;
class OrthographicCamera;
class BatchManager;
class SSBOManager;
class BatchStatistics;
class Scene3DPassContext;
using Scene3DPassContextSharedPtr = std::shared_ptr<Scene3DPassContext>;

struct FrameState {
    /// 3D legacy 镜像成员（lighting/IBL/frameUBO），实现内部定义。
    struct Scene3DLegacy;

    FrameState();
    ~FrameState();

    /// 拷贝赋值仅复制公共字段（Scene3DLegacy 为内部实现，不参与拷贝）。
    FrameState& operator=(const FrameState& other);

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
    ClipRect currentClip;
    std::vector<ClipRect> clipStack;
    int32_t framebufferWidth = 0;
    int32_t framebufferHeight = 0;
    // 窗口清屏色镜像（beginRenderPass 写入）：backdrop RT 需用同一颜色清屏，
    // 回屏合成后才能与无模糊路径的背景一致（KAWASE_BACKDROP_BLUR_PROPOSAL.md §5.1）。
    Vector4 clearColor;

    uint32_t drawCallCount = 0;
    uint32_t fps = 0;
    bool isSSBOSupport = false;
    std::shared_ptr<BatchStatistics> batchStatistics;

    std::shared_ptr<PerspectiveCamera> perspectiveCamera;  // null=2D path
    Scene3DPassContextSharedPtr scene3DPassContext;
    // Legacy mirrors kept temporarily while 3D code migrates to scene3DPassContext.
    std::unique_ptr<Scene3DLegacy> scene3DLegacy;
};

using FrameStateSharedPtr = std::shared_ptr<FrameState>;
}

#endif //MORROW_FRAMESTATE_H
