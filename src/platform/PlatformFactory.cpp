//
// Created by 0060328 on 25-9-18.
//

#include "PlatformFactory.h"

#include <memory>

#ifdef OPENGL_EGL
#include "egl/QNXPlatform.h"
#elif defined(OPENGL_GLFW)
#include "wgl/WGLPlatform.h"
#endif

namespace morrow {
PlatformSharedPtr PlatformFactory::create(const WindowInfo& info) noexcept {
#ifdef OPENGL_EGL
    return std::make_shared<QNXPlatform>(info);
#elif defined(OPENGL_GLFW)
    return std::make_shared<WGLPlatform>(info);
#endif
    return nullptr;
}

void PlatformFactory::destroy(Platform** platform) noexcept {
}
} // morrow