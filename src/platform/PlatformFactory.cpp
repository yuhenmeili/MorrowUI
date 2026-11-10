//
// Created by 0060328 on 25-9-18.
//

#include "PlatformFactory.h"

#include <memory>

#include "morrow/utils/Log.h"

#ifdef OPENGL_EGL
#include "egl/QNXPlatform.h"
#elif defined(OPENGL_GLFW)
#include "wgl/WGLPlatform.h"
#endif

namespace morrow {
PlatformSharedPtr PlatformFactory::create(const WindowInfo& info,
                                          const std::vector<WindowInfo>& extraWindowInfos,
                                          ScreenRenderMode screenMode) noexcept {
#ifdef OPENGL_EGL
    return std::make_shared<QNXPlatform>(info, extraWindowInfos, screenMode);
#elif defined(OPENGL_GLFW)
    (void)screenMode;
    if (!extraWindowInfos.empty()) {
        // Windows (GLFW) 平台暂不支持多窗口：extraWindowInfos 被忽略，
        // 保持单窗口行为（见 docs/DUAL_DISPLAY_FEASIBILITY.md 实现记录）
        LOG_W("extraWindowInfos ignored on GLFW platform: {} window(s) dropped",
              extraWindowInfos.size());
    }
    return std::make_shared<WGLPlatform>(info);
#endif
    return nullptr;
}

void PlatformFactory::destroy(Platform** platform) noexcept {
}
} // morrow
