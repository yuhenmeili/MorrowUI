#ifndef MORROW_ENGINE_H_
#define MORROW_ENGINE_H_

#include "FrameState.h"
#include "OrthographicCamera.h"
#include "FPSController.h"
#include "Platform.h"
#include "Window.h"

namespace morrow
{
class DebugPlane;
struct FontInfo;

struct EngineOptions {
    bool multithread = true;
    bool enableRequestRender = false;
    int32_t samples = 1;
    WindowInfo windowInfo = {};
};

class Engine
{
public:
    explicit Engine(const EngineOptions& options = {});

    virtual ~Engine();

    [[nodiscard]] WindowSharedPtr getWindow() const;

    void addFonts(const std::vector<FontInfo>& fontsUrl);

    void setFPS(int32_t fps);

    Observable<>& preRender();

    void render();

    Observable<>& afterRender();

private:
    void updateFrameState();

    void callAfterRenderFunctions();

    void heartbeat();

    Observable<> m_preRender;
    Observable<> m_afterRender;

    //debug
    double m_lastHeartbeatTime = 0.0;
    uint64_t m_lastHeartbeatFrameNumber = 0;

    FrameStateSharedPtr m_frameState;
    OrthographicCameraSharedPtr m_camera;
    double m_monotonicTime = 0.0;
    //manager
    FPSControllerPtr m_fpsController;
    bool m_requestRenderEnabled = false;

    PlatformSharedPtr m_platform;
    std::shared_ptr<DebugPlane> m_debugPlane;
};

using EngineSharedPtr = std::shared_ptr<Engine>;
}

#endif /* MORROW_ENGINE_H_ */
