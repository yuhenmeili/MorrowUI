//
// Created by lance on 2026/4/21.
//

#ifndef MORROW_GUI_SCENE3DASYNCLOADER_H
#define MORROW_GUI_SCENE3DASYNCLOADER_H

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "Engine.h"
#include "../elements/MR3DSceneView.h"
#include "GLTFTypes.h"

namespace morrow {
class SceneNode;

struct Scene3DAsyncLoadResult {
    std::shared_ptr<SceneNode> sceneRoot;
    bool hasBounds = false;
    Vector3 boundsMin = {};
    Vector3 boundsMax = {};
    std::string error;
};

struct Scene3DAsyncLoadOptions {
    std::string debugLabel;
    Scene3DCameraFitOptions cameraFit{};
    std::function<void()> onStarted;
    std::function<void(const Scene3DAsyncLoadResult&)> onLoaded;
    std::function<void(const std::string&)> onError;
};

class Scene3DAsyncLoader : public std::enable_shared_from_this<Scene3DAsyncLoader> {
public:
    using WorkerPayload = std::shared_ptr<void>;
    using Completion = std::function<void(WorkerPayload payload, const std::string& error)>;
    using AsyncLoadStarter = std::function<void(Completion completion)>;
    using MainThreadApply = std::function<Scene3DAsyncLoadResult(const WorkerPayload& payload)>;

    enum class Status {
        Idle,
        Loading,
        Succeeded,
        Failed,
        Cancelled,
    };

    struct GLTFLoadOptions {
        std::string shaderName = "gltf_pbr";
        bool autoPlayFirstAnimation = true;
        Scene3DAsyncLoadOptions sceneOptions;
        std::function<void(const std::shared_ptr<GLTFScene>& scene,
                           const std::shared_ptr<SceneNode>& sceneRoot)> onSceneBuilt;
    };

    static std::shared_ptr<Scene3DAsyncLoader> create(const EngineSharedPtr& engine,
                                                      const std::shared_ptr<MR3DSceneView>& scene3DView);

    ~Scene3DAsyncLoader();

    void load(const AsyncLoadStarter& asyncLoadStarter,
              MainThreadApply mainThreadApply,
              Scene3DAsyncLoadOptions options = Scene3DAsyncLoadOptions());

    void loadGLTF(const std::string& modelPath);

    void loadGLTF(const std::string& modelPath,
                  const GLTFLoadOptions& options);

    void cancel();

    bool isLoading() const;

    Status getStatus() const;

    std::string getLastError() const;

private:
    Scene3DAsyncLoader(EngineSharedPtr engine, const std::shared_ptr<MR3DSceneView>& scene3DView);

    void attachPreRenderObserver();

    void detachPreRenderObserver();

    void handleWorkerCompletion(uint64_t generation, WorkerPayload payload, const std::string& error);

    void pumpPendingResult();

    mutable std::mutex m_mutex;
    EngineSharedPtr m_engine;
    std::weak_ptr<MR3DSceneView> m_scene3DView;
    std::string m_preRenderObserverId;

    uint64_t m_generation = 0;
    bool m_loading = false;
    bool m_ready = false;
    Status m_status = Status::Idle;
    WorkerPayload m_pendingPayload;
    std::string m_pendingError;
    MainThreadApply m_mainThreadApply;
    Scene3DAsyncLoadOptions m_activeOptions;
    std::string m_lastError;
};
} // namespace morrow

#endif //MORROW_GUI_SCENE3DASYNCLOADER_H
