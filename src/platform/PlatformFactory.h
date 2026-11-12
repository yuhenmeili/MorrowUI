//
// Created by 0060328 on 25-9-18.
//

#ifndef PLATFORMFACTORY_H
#define PLATFORMFACTORY_H
#include <string>
#include <vector>

#include "Platform.h"
#include "PlatformWindow.h"

namespace morrow {
class PlatformFactory {
public:
    /// info 为主窗口；extraWindowInfos 为第二块屏起的多窗口配置
    ///（当前仅 QNX/EGL 平台支持，其他平台忽略并打日志）；
    /// screenMode 控制多屏渲染模式（Mirror 默认 / Independent）
    static PlatformSharedPtr create(const WindowInfo& info,
                                    const std::vector<WindowInfo>& extraWindowInfos = {},
                                    ScreenRenderMode screenMode = ScreenRenderMode::Mirror) noexcept;

    static void destroy(Platform** platform) noexcept;
};

} // morrow

#endif //PLATFORMFACTORY_H
