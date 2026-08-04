#include "Engine.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "FontManager.h"
#include "MathUtils.h"
#include "ui/base/Widget.h"
#include "GlobalObject.h"
#include "PlatformFactory.h"
#include "debug/DebugPlane.h"
#include "ui/helpers/Tween.h"
#include "debug/ObjectRegistry.h"
#include "renderer/device/RenderDeviceProxy.h"

namespace {
std::filesystem::path makeFrameTaggedSnapshotPath(const std::string& requestedPath,
                                                  uint64_t frame) {
    const std::filesystem::path outputPath(requestedPath);
    const auto parentPath = outputPath.parent_path();
    const auto stem = outputPath.stem().string();
    const auto extension = outputPath.extension().string();
    const auto frameTaggedStem = stem + "-frame-" + std::to_string(frame);

    auto candidate = parentPath / (frameTaggedStem + extension);
    std::error_code error;
    for (uint32_t sequence = 1; std::filesystem::exists(candidate, error) && !error; ++sequence) {
        candidate = parentPath / (frameTaggedStem + "-" + std::to_string(sequence) + extension);
    }
    return candidate;
}
}

namespace morrow {
Engine::Engine(const EngineOptions& options) {
    WindowInfo windowInfo = options.windowInfo;
    windowInfo.samples = std::max(options.samples, 1);

    LOG_I("multithread {}, enableRequestRender {}, samples {}",
          options.multithread,
          options.enableRequestRender,
          windowInfo.samples);
    m_requestRenderEnabled = options.enableRequestRender;
    m_maxFrames = options.maxFrames;
    m_objectSnapshotPath = options.objectSnapshotPath;
    m_objectSnapshotCommandPath = options.objectSnapshotCommandPath;
    m_platform = PlatformFactory::create(windowInfo);
    m_platform->initialize(options.multithread);

    m_camera = std::make_shared<OrthographicCamera>(0.0f, 11000.0f);
    m_camera->setPosition(0.0f, 0.0f, 10000.0f);
    m_camera->setDirection(0.0f, 0.0f, -1.0f);
    m_camera->setUp(0.0f, 1.0f, 0.0f);

    m_fpsController = std::make_shared<FPSController>(30);
    m_frameState = std::make_shared<FrameState>();
    // m_frameState->esContext = m_esContext;
    m_frameState->camera = m_camera;
    m_frameState->inputEventsManager = m_platform->getInputManager();
    m_debugPlane = std::make_shared<DebugPlane>();
    m_debugPlane->initialize(m_platform->getWindow());
}

Engine::~Engine() {
    GlobalObject::getInstance().destroy();
}

WindowSharedPtr Engine::getWindow() const {
    return m_platform ? m_platform->getWindow() : nullptr;
}

FrameStateSharedPtr Engine::getFrameState() const {
    return m_frameState;
}

void Engine::addFonts(const std::vector<FontInfo>& fontsUrl) {
    GlobalObject::getInstance().getFontManager()->addFonts(fontsUrl);
}

void Engine::setFPS(int32_t fps) {
    m_fpsController->setFPS(fps);
}

Observable<>& Engine::preRender() {
    return m_preRender;
}

void Engine::render() {
    m_lastHeartbeatTime = Math::getCurrentMonotonicTime();
    uint32_t renderedFrameCount = 0;
    while (!m_platform->shouldClose()) {
        updateFrameState();

        // ── 阶段 1: 输入 ──
        if (!m_platform->beginFrame(m_frameState)) {
            break;
        }
        m_platform->dispatchEvents(m_frameState);

        // ── 阶段 2: 动画准备 ──
        m_preRender.notify();
        TweenManager::getInstance().update(m_frameState);

        // ── 阶段 3: 渲染管线 ──
        m_platform->beginRenderPass(m_frameState);      // GPU 准备
        m_platform->updateWidgets(m_frameState);         // Widget 树遍历 (standard update)
        m_platform->lateUpdateWidgets(m_frameState);     // Widget 树 lateUpdate (依赖其他组件已更新)
        m_platform->commitRenderPass(m_frameState);      // GPU 提交

        // ── 阶段 4: 帧后处理 ──
        m_frameState->frameNumber++;
        ObjectRegistry::getInstance().setCurrentFrame(m_frameState->frameNumber);
        heartbeat();
        m_debugPlane->update(m_frameState);
        m_afterRender.notify();
        m_platform->endFrame();

        callAfterRenderFunctions();
        ++renderedFrameCount;
        if (m_maxFrames > 0 && renderedFrameCount >= m_maxFrames) {
            break;
        }
    }

    if (!m_objectSnapshotPath.empty()) {
        writeObjectSnapshot(m_objectSnapshotPath);
    }
    m_platform->terminate();
}

Observable<>& Engine::afterRender() {
    return m_afterRender;
}

bool Engine::writeObjectSnapshot(const std::string& path) const {
    const auto root = m_platform ? m_platform->getWindow() : nullptr;
    if (!root || path.empty()) return false;
    root->refreshDebugObjectTree();
    const uint64_t frame = m_frameState ? m_frameState->frameNumber : 0;
    const auto outputPath = makeFrameTaggedSnapshotPath(path, frame);
    if (ObjectRegistry::getInstance().writeSnapshot(outputPath.string(), frame, root->getDebugObjectId())) {
        LOG_I("Object snapshot written: {}", outputPath.string());
        return true;
    }
    LOG_E("Failed to write object snapshot: {}", outputPath.string());
    return false;
}

void Engine::processObjectSnapshotCommand() {
    if (m_objectSnapshotCommandPath.empty()) return;

    std::ifstream input(m_objectSnapshotCommandPath);
    if (!input.is_open()) return;

    std::string command;
    std::getline(input, command);
    input.close();
    if (command.size() >= 3 &&
        static_cast<unsigned char>(command[0]) == 0xEF &&
        static_cast<unsigned char>(command[1]) == 0xBB &&
        static_cast<unsigned char>(command[2]) == 0xBF) {
        command.erase(0, 3);
    }
    const auto first = command.find_first_not_of(" \t\r\n");
    const auto last = command.find_last_not_of(" \t\r\n");
    command = first == std::string::npos
                  ? std::string{}
                  : command.substr(first, last - first + 1);

    // Consume each command at most once. Writers should publish atomically
    // (temporary file + rename) if they can race with the heartbeat.
    std::error_code error;
    std::filesystem::remove(m_objectSnapshotCommandPath, error);

    constexpr const char* prefix = "snapshot";
    if (command.compare(0, std::char_traits<char>::length(prefix), prefix) != 0) {
        LOG_W("Unknown object snapshot command: {}", command);
        return;
    }

    std::string path;
    const auto pathPosition = command.find("path=");
    if (pathPosition != std::string::npos) {
        path = command.substr(pathPosition + 5);
    } else {
        std::istringstream stream(command);
        std::string ignored;
        stream >> ignored >> path;
    }
    const auto pathFirst = path.find_first_not_of(" \t\r\n");
    const auto pathLast = path.find_last_not_of(" \t\r\n");
    path = pathFirst == std::string::npos
               ? std::string{}
               : path.substr(pathFirst, pathLast - pathFirst + 1);

    if (path.empty()) {
        LOG_W("Object snapshot command requires a path");
        return;
    }
    writeObjectSnapshot(path);
}

void Engine::updateFrameState() {
    double currentTime = Math::getCurrentMonotonicTime();
    if (m_monotonicTime <= 0) {
        //first frame
        m_monotonicTime = currentTime;
    }
    m_frameState->deltaTime = currentTime - m_monotonicTime;
    if (m_frameState->deltaTime < 0.0) m_frameState->deltaTime = 0.0;
    m_monotonicTime = currentTime;
    m_frameState->callAfterRender.clear();
    m_frameState->callAfterTouched.clear();
    m_frameState->debugWidgetsAfterAnimate.clear();
    m_frameState->drawCallCount = 0;
    m_frameState->batchStatistics.reset();
    m_frameState->batchManager = nullptr;
    m_frameState->ssboManager = GlobalObject::getInstance().getSSBOManager();
}

void Engine::callAfterRenderFunctions() {
    for (const auto& callback : m_frameState->callAfterRender) {
        callback();
    }
    for (const auto& widget : m_frameState->debugWidgetsAfterAnimate) {
        widget->debugTraversal("debugWidgetsAfterAnimate");
    }
}

void Engine::heartbeat() {
    double deltaTime = m_monotonicTime - m_lastHeartbeatTime;
    auto logGap = 2.0f;
    if (deltaTime >= logGap) {
        m_lastHeartbeatTime = m_monotonicTime;
        int32_t fps = 30;
        if (m_frameState->frameNumber >= m_lastHeartbeatFrameNumber) {
            uint32_t frameNumber = m_frameState->frameNumber - m_lastHeartbeatFrameNumber;
            fps = std::ceil((double)frameNumber / deltaTime);
        }
        m_lastHeartbeatFrameNumber = m_frameState->frameNumber;
        m_frameState->fps = fps;
        processObjectSnapshotCommand();
        // LOG_I("Render FPS : {}", fps);
    } else if (deltaTime < 0) {
        m_lastHeartbeatTime = m_monotonicTime;
        LOG_E("Time hopping");
    }
}
}
