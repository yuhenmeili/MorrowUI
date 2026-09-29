// FrameStateLegacy.h — FrameState::Scene3DLegacy 的内部定义（引擎私有）。
// 3D 代码迁移到 scene3DPassContext 之前的镜像成员，收敛后随迁移删除。
#pragma once

#include "morrow/ResourceHandle.h"
#include "morrow/scene3d/FrameState.h"
#include "Scene3DPassContext.h"

namespace morrow {

struct FrameState::Scene3DLegacy {
    Scene3DLightingState lighting;
    Scene3DIBLState ibl;
    HwUBO scene3DFrameUBO{0};
};

} // namespace morrow
