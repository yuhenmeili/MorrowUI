//
// WindowInfo — 窗口创建配置（公共）。
//
// 窗口实现为引擎内部对象（见 docs/road_map/PUBLIC_API_ENCAPSULATION_PROPOSAL.md
// §6.7）：应用经 EngineOptions::windowInfo 配置主窗口，经 Engine::getWindow(index)
// 取得窗口门面 morrow::Window 操作运行时能力。
//

#ifndef MORROW_WINDOWINFO_H
#define MORROW_WINDOWINFO_H

#include <cstdint>
#include <memory>
#include <string>

namespace morrow {

enum class CursorShape {
    Arrow,
    IBeam,
    Hand,
    ResizeHorizontal,
    ResizeVertical,
    ResizeAll,
    Forbidden,
};

enum WindowMask : int32_t {
    SCREEN_SENSITIVITY_MASK_ALWAYS = (1 << 0),
    SCREEN_SENSITIVITY_MASK_NEVER = (2 << 0),
    SCREEN_SENSITIVITY_MASK_NO_FOCUS = (1 << 3),
    SCREEN_SENSITIVITY_MASK_FULLSCREEN = (1 << 4),
    SCREEN_SENSITIVITY_MASK_CONTINUE = (1 << 5),
    SCREEN_SENSITIVITY_MASK_STOP = (2 << 5),
    SCREEN_SENSITIVITY_MASK_POINTER_BRUSH = (1 << 7),
    SCREEN_SENSITIVITY_MASK_FINGER_BRUSH = (1 << 8),
    SCREEN_SENSITIVITY_MASK_STYLUS_BRUSH = (1 << 9),
    SCREEN_SENSITIVITY_MASK_OVERDRIVE = (1 << 10),
    SCREEN_SENSITIVITY_MASK_CLIPPED = (1 << 11),
};

/// 多屏渲染模式（仅当配置了多块屏时生效）：
/// Mirror —— UI 只渲染一次到离屏 RT，各屏贴图上屏（等比缩放 + 黑边适配
/// 分辨率差异，内容分辨率取主窗口尺寸）；
/// Independent —— 每个窗口（displayId）各自渲染各自的 Scene2D 子树。
enum class ScreenRenderMode : int32_t {
    Mirror = 0,
    Independent = 1,
};

struct WindowInfo {
    virtual ~WindowInfo() = default;
    std::string name = "default";
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 1920;
    int32_t height = 1080;
    int32_t samples = 1;
    int32_t alpha = 1.0f;
    //egl
    int32_t displayId = 3;
    int32_t zorder = 4000;
    WindowMask sensitivity = WindowMask::SCREEN_SENSITIVITY_MASK_ALWAYS;
};

using WindowInfoSharedPtr = std::shared_ptr<WindowInfo>;

} // namespace morrow

#endif //MORROW_WINDOWINFO_H
