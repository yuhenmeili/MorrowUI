#ifndef MORROW_ENGINE_H_
#define MORROW_ENGINE_H_

#include "FrameState.h"
#include "EngineEvents.h"
#include "MainThreadDispatcher.h"
#include "OrthographicCamera.h"
#include "FPSController.h"
#include "Platform.h"
#include "Window.h"
#include "renderer/device/RenderDeviceOptions.h"
#include <string>

namespace morrow
{
class DebugPlane;
struct FontInfo;

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
};

class Engine
{
public:
    explicit Engine(const EngineOptions& options = {});

    virtual ~Engine();

    [[nodiscard]] WindowSharedPtr getWindow() const;

    [[nodiscard]] FrameStateSharedPtr getFrameState() const;

    void addFonts(const std::vector<FontInfo>& fontsUrl);

    void setFPS(int32_t fps);

    EngineEvents& events();

    MainThreadDispatcher& mainThreadDispatcher();

    void setDebugOverlayVisible(bool visible);

    void toggleDebugOverlay();

    [[nodiscard]] bool isDebugOverlayVisible() const;

    void render();

    bool writeObjectSnapshot(const std::string& path) const;

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

    EngineEvents m_events;
    MainThreadDispatcher m_mainThreadDispatcher;

    //debug
    double m_lastHeartbeatTime = 0.0;
    uint64_t m_lastHeartbeatFrameNumber = 0;

    FrameStateSharedPtr m_frameState;
    OrthographicCameraSharedPtr m_camera;
    double m_monotonicTime = 0.0;
    //manager
    FPSControllerPtr m_fpsController;
    bool m_requestRenderEnabled = false;
    uint32_t m_maxFrames = 0;
    std::string m_objectSnapshotPath;
    std::string m_objectSnapshotCommandPath;

    PlatformSharedPtr m_platform;
    std::shared_ptr<DebugPlane> m_debugPlane;
};

using EngineSharedPtr = std::shared_ptr<Engine>;
}

#endif /* MORROW_ENGINE_H_ */
