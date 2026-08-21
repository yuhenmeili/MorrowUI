#ifndef MORROW_ENGINE_H_
#define MORROW_ENGINE_H_

#include "FrameState.h"
#include "EngineEvents.h"
#include "MainThreadDispatcher.h"
#include "OrthographicCamera.h"
#include "FPSController.h"
#include "Platform.h"
#include "Window.h"
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
    std::string objectSnapshotPath;
    std::string objectSnapshotCommandPath;
    WindowInfo windowInfo = {};
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

    void render();

    bool writeObjectSnapshot(const std::string& path) const;

private:
    void updateFrameState();

    void callAfterRenderFunctions();

    void heartbeat();

    void processObjectSnapshotCommand();

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
