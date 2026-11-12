#ifndef MORROW_ENGINE_H_
#define MORROW_ENGINE_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "morrow/base/TouchEvent.h"
#include "morrow/base/Scene2D.h"
#include "morrow/EngineEvents.h"
#include "morrow/FontInfo.h"
#include "morrow/OrthographicCamera.h"
#include "morrow/RenderDeviceOptions.h"
#include "morrow/Window.h"
#include "morrow/WindowInfo.h"
#include "morrow/effects/BackdropBlur.h"
#include "morrow/scene3d/FrameState.h"

namespace morrow
{
class DebugPlane;
class Platform;
class MainThreadDispatcher;
class FPSController;

struct EngineOptions {
    bool multithread = true;
    bool enableRequestRender = false;
    int32_t samples = 1;
    uint32_t maxFrames = 0; // 0 = run until the platform window closes
    bool debugOverlayVisible = false;
    std::string objectSnapshotPath;
    std::string objectSnapshotCommandPath;
    WindowInfo windowInfo = {};
    /// 多屏配置：第二块屏起的窗口（displayId 须与主窗口不同）。仅 QNX/EGL
    /// 平台生效；其他平台忽略并打日志。
    std::vector<WindowInfo> extraWindowInfos;
    /// 多屏渲染模式开关（仅配置了多块屏时生效）：
    /// Mirror（默认）= UI 只渲染一次到离屏 RT，各屏贴图上屏（等比缩放 + 黑边）；
    /// Independent = 每块屏各自渲染各自的 Scene2D 子树（经 getScene2D(index)）。
    ScreenRenderMode screenMode = ScreenRenderMode::Mirror;

    // ── 启动速度 / 轻量化配置 ──
    /// 渲染设备启动配置：CommandBuffer 容量、shader 二进制缓存目录、
    /// SSBO 能力预设。shaderBinaryCacheDir 为空时按平台取默认
    /// （QNX: "/var/data/shaders"，桌面平台: 关闭）。
    RenderDeviceOptions deviceOptions;
    /// 跳过引擎默认字体（约 8.4MB 同步读盘）。应用通过 addFonts 提供
    /// 自己的字体时置 true。
    bool skipDefaultFont = false;
    /// 默认字体异步预读（与 EGL 初始化并行），关闭则回退同步读盘。
    bool asyncFontPreload = true;
    /// 字形图集初始边长（像素）。0 = 引擎默认 1024；CJK 首屏文案多时
    /// 建议 2048，避免首帧图集扩容触发全量字形重建。
    int32_t fontAtlasInitialSize = 0;

    // ── 背景模糊（KAWASE_BACKDROP_BLUR_PROPOSAL.md §5.7）──
    /// 启动档位：Off = 全程退化路径（RT 链不分配，毛玻璃降级为 tint 面板）；
    /// Standard = 1/2 基准链；LowCost = 1/4 基准链。运行中可经
    /// setBackdropBlurQuality 切换（懒重建）。
    BackdropBlurQuality backdropBlur = BackdropBlurQuality::Standard;
};

// ---------------------------------------------------------------------------
// Engine — 引擎入口。
//
// Platform 为引擎内部对象；窗口经公共门面 morrow::Window 暴露（§6.7 修订，
// 多屏后 Engine 不再是单窗口 facade）：应用经 EngineOptions::windowInfo 配置
// 主窗口（多屏另配 extraWindowInfos + screenMode，见 WindowInfo.h 的
// ScreenRenderMode），经 getWindow(index) 持有各窗口——清屏色 / 剪贴板 /
// 光标 / framebuffer 查询 / 各窗口事件 / Scene2D 根都在 Window 上
//（getWindow(i)->scene2D() 搭建界面）。Engine 只保留引擎全局能力：字体 /
// FPS / 背景模糊档位 / 调试覆盖层 / EngineEvents（仅转播主窗口的原始键盘
// 与 framebuffer 事件）/ 主线程队列 / 渲染循环。
// ---------------------------------------------------------------------------
class Engine
{
public:
    explicit Engine(const EngineOptions& options = {});

    virtual ~Engine();

    /// 窗口门面（0 = 主窗口，序号即 extraWindowInfos 配置顺序）。
    /// 越界或平台初始化失败（如非 QNX 平台丢弃了 extraWindowInfos）返回
    /// nullptr；重复取同一 index 返回同一实例。
    std::shared_ptr<Window> getWindow(int32_t index = 0) const;

    /// 窗口总数（主窗口 + extraWindowInfos 中被平台接受的窗口）。
    [[nodiscard]] int32_t windowCount() const;

    /// 不透明帧上下文（deltaTime / 帧号等只读视图）。
    [[nodiscard]] FrameStateSharedPtr getFrameState() const;

    void addFonts(const std::vector<FontInfo>& fontsUrl);

    void setFPS(int32_t fps);

    /// 背景模糊运行档位（懒重建，见 EngineOptions::backdropBlur）。
    void setBackdropBlurQuality(BackdropBlurQuality quality);

    /// 背景模糊运行时开关/档位查询（毛玻璃降级为 tint 面板）。
    void setBackdropBlurEnabled(bool enabled);

    [[nodiscard]] bool isBackdropBlurEnabled() const;

    [[nodiscard]] BackdropBlurQuality getBackdropBlurQuality() const;

    EngineEvents& events();

    /// 主线程一次性任务队列（worker → Engine 主线程）。
    MainThreadDispatcher& mainThreadDispatcher();

    void setDebugOverlayVisible(bool visible);

    void toggleDebugOverlay();

    [[nodiscard]] bool isDebugOverlayVisible() const;

    void render();

    bool writeObjectSnapshot(const std::string& path) const;

private:
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    EngineEvents m_events;
    std::unique_ptr<MainThreadDispatcher> m_mainThreadDispatcher;

    //debug
    double m_lastHeartbeatTime = 0.0;
    uint64_t m_lastHeartbeatFrameNumber = 0;

    FrameStateSharedPtr m_frameState;
    OrthographicCameraSharedPtr m_camera;
    double m_monotonicTime = 0.0;
    //manager
    std::shared_ptr<FPSController> m_fpsController;
    bool m_requestRenderEnabled = false;
    uint32_t m_maxFrames = 0;
    std::string m_objectSnapshotPath;
    std::string m_objectSnapshotCommandPath;

    std::shared_ptr<Platform> m_platform;
    std::shared_ptr<DebugPlane> m_debugPlane;

    /// getWindow(index) 的门面缓存（与 Platform::getWindows() 序号对齐）
    mutable std::vector<std::shared_ptr<Window>> m_windowFacades;

    // 内部 WindowEvents → EngineEvents 转播连接
    Observable<const TouchEvent&>::Connection m_rawKeyBridge;
    Observable<const Vector2&>::Connection m_framebufferBridge;

private:
    void updateFrameState();

    void callAfterRenderFunctions();

    void heartbeat();

    void processObjectSnapshotCommand();

    void processDebugShortcuts();

#if MORROW_ENABLE_DEBUG_OVERLAY
    /// DebugPlane 懒创建：首帧构造 2 个 MRLabel 并参与布局，即使不可见也有
    /// 启动成本；推迟到首次 setVisible(true)/toggle 时创建。
    DebugPlane* ensureDebugPlane();
#endif
};

using EngineSharedPtr = std::shared_ptr<Engine>;
}

#endif /* MORROW_ENGINE_H_ */
