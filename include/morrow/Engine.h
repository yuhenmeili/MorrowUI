#ifndef MORROW_ENGINE_H_
#define MORROW_ENGINE_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "morrow/base/TouchEvent.h"
#include "morrow/base/Root2D.h"
#include "morrow/EngineEvents.h"
#include "morrow/FontInfo.h"
#include "morrow/OrthographicCamera.h"
#include "morrow/RenderDeviceOptions.h"
#include "morrow/WindowInfo.h"
#include "morrow/effects/BackdropBlur.h"
#include "morrow/scene3d/FrameState.h"

namespace morrow
{
class DebugPlane;
class Platform;
class Window;
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
// Engine — 引擎入口（单窗口假设）。
//
// Window / Platform 为引擎内部对象（§6.7）：应用经 EngineOptions::windowInfo
// 配置窗口，经 getRootWidget() 取得 2D UI 根搭建界面；窗口清屏色与原始键盘
// 输入、framebuffer 尺寸变化经下方公共接口/事件获得。
// ---------------------------------------------------------------------------
class Engine
{
public:
    explicit Engine(const EngineOptions& options = {});

    virtual ~Engine();

    /// 2D UI 根节点：向其 addChild 搭建界面。
    [[nodiscard]] Root2DSharedPtr getRootWidget() const;

    /// 不透明帧上下文（deltaTime / 帧号等只读视图）。
    [[nodiscard]] FrameStateSharedPtr getFrameState() const;

    void addFonts(const std::vector<FontInfo>& fontsUrl);

    void setFPS(int32_t fps);

    /// 窗口清屏色。
    void setClearColor(float r, float g, float b, float a);

    /// framebuffer 尺寸（像素）。窗口未就绪时返回 {0, 0}。
    [[nodiscard]] Vector2 framebufferSize() const;

    /// 系统剪贴板读写（平台不支持时为空操作/空串）。
    void setClipboardText(const std::string& text);

    [[nodiscard]] std::string clipboardText() const;

    /// 鼠标指针形态。
    void setCursorShape(CursorShape shape);

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
