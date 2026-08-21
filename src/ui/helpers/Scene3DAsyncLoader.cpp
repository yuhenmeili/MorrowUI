//
// Created by lance on 2026/4/21.
//

#include "Scene3DAsyncLoader.h"

#include <utility>

#include "GLTFAnimationController.h"
#include "GLTFLoader.h"
#include "GLTFSceneBuilder.h"
#include "Log.h"

namespace morrow {
std::shared_ptr<Scene3DAsyncLoader> Scene3DAsyncLoader::create(const EngineSharedPtr& engine,
                                                               const std::shared_ptr<MR3DSceneView>& scene3DView) {
    auto loader = std::shared_ptr<Scene3DAsyncLoader>(new Scene3DAsyncLoader(engine, scene3DView));
    loader->attachFrameBeginObserver();
    return loader;
}

Scene3DAsyncLoader::Scene3DAsyncLoader(EngineSharedPtr engine,
                                       const std::shared_ptr<MR3DSceneView>& scene3DView)
    : m_engine(std::move(engine)),
      m_scene3DView(scene3DView) {
}

Scene3DAsyncLoader::~Scene3DAsyncLoader() {
    cancel();
    detachFrameBeginObserver();
}

void Scene3DAsyncLoader::attachFrameBeginObserver() {
    if (!m_engine || m_frameBeginConnection.connected()) return;

    std::weak_ptr<Scene3DAsyncLoader> weakSelf = shared_from_this();
    m_frameBeginConnection = m_engine->events().onFrameBegin.connect([weakSelf]() {
        if (auto self = weakSelf.lock()) {
            self->pumpPendingResult();
        }
    });
}

void Scene3DAsyncLoader::detachFrameBeginObserver() {
    m_frameBeginConnection.disconnect();
}

void Scene3DAsyncLoader::load(const AsyncLoadStarter& asyncLoadStarter,
                              MainThreadApply mainThreadApply,
                              Scene3DAsyncLoadOptions options) {
    if (!asyncLoadStarter || !mainThreadApply) {
        const std::string error = "Scene3DAsyncLoader requires both asyncLoadStarter and mainThreadApply";
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_loading = false;
            m_ready = false;
            m_status = Status::Failed;
            m_lastError = error;
            m_pendingPayload.reset();
            m_pendingError.clear();
            m_mainThreadApply = {};
            m_activeOptions = {};
        }
        LOG_E("Scene3DAsyncLoader: {}", error);
        if (options.onError) options.onError(error);
        return;
    }

    if (!m_engine) {
        const std::string error = "Scene3DAsyncLoader requires a valid Engine instance";
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_loading = false;
            m_ready = false;
            m_status = Status::Failed;
            m_lastError = error;
            m_pendingPayload.reset();
            m_pendingError.clear();
            m_mainThreadApply = {};
            m_activeOptions = {};
        }
        LOG_E("Scene3DAsyncLoader: {}", error);
        if (options.onError) options.onError(error);
        return;
    }

    cancel();
    attachFrameBeginObserver();

    uint64_t generation = 0;
    auto onStarted = options.onStarted;
    auto debugLabel = options.debugLabel;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ++m_generation;
        generation = m_generation;
        m_loading = true;
        m_ready = false;
        m_status = Status::Loading;
        m_pendingPayload.reset();
        m_pendingError.clear();
        m_mainThreadApply = std::move(mainThreadApply);
        m_activeOptions = std::move(options);
        m_lastError.clear();
    }

    if (!debugLabel.empty()) {
        LOG_I("Scene3DAsyncLoader [{}]: start generation {}", debugLabel, generation);
    } else {
        LOG_I("Scene3DAsyncLoader: start generation {}", generation);
    }

    if (onStarted) onStarted();

    std::weak_ptr<Scene3DAsyncLoader> weakSelf = shared_from_this();
    asyncLoadStarter([weakSelf, generation](WorkerPayload payload, const std::string& error) mutable {
        if (auto self = weakSelf.lock()) {
            self->handleWorkerCompletion(generation, std::move(payload), error);
        }
    });
}

void Scene3DAsyncLoader::loadGLTF(const std::string& modelPath) {
    loadGLTF(modelPath, GLTFLoadOptions{});
}

void Scene3DAsyncLoader::loadGLTF(const std::string& modelPath,
                                  const GLTFLoadOptions& options) {
    GLTFLoadOptions resolvedOptions = options;
    auto shaderName = resolvedOptions.shaderName.empty() ? std::string("gltf_pbr") : resolvedOptions.shaderName;
    auto onSceneBuilt = std::move(resolvedOptions.onSceneBuilt);
    const bool autoPlayFirstAnimation = resolvedOptions.autoPlayFirstAnimation;
    const Scene3DNormalizationOptions normalization = resolvedOptions.normalization;
    auto sceneOptions = std::move(resolvedOptions.sceneOptions);
    if (sceneOptions.debugLabel.empty()) {
        sceneOptions.debugLabel = modelPath;
    }

    load(
        [modelPath](Completion completion) {
            GLTFLoader::loadAsync(modelPath, [completion = std::move(completion)](const std::shared_ptr<GLTFScene>& scene,
                                                                                  const std::string& error) mutable {
                completion(std::static_pointer_cast<void>(scene), error);
            });
        },
        [shaderName, autoPlayFirstAnimation, normalization, onSceneBuilt](const WorkerPayload& payload) -> Scene3DAsyncLoadResult {
            Scene3DAsyncLoadResult result;
            auto gltfScene = std::static_pointer_cast<GLTFScene>(payload);
            if (!gltfScene) {
                result.error = "GLTF loader returned an empty scene";
                return result;
            }

            result.sceneRoot = GLTFSceneBuilder::build(gltfScene, shaderName);
            if (!result.sceneRoot) {
                result.error = "GLTFSceneBuilder failed to build the scene";
                return result;
            }

            result.hasBounds = GLTFSceneBuilder::computeBounds(gltfScene, result.boundsMin, result.boundsMax);
            if (result.hasBounds && normalization.enabled) {
                const Scene3DNormalizationResult normalized =
                    calculateScene3DNormalization(
                        result.boundsMin,
                        result.boundsMax,
                        normalization);
                if (normalized.applied) {
                    if (auto transform = result.sceneRoot->getTransform()) {
                        transform->setLocalScale(
                            normalized.uniformScale,
                            normalized.uniformScale,
                            normalized.uniformScale);
                        result.boundsMin = normalized.boundsMin;
                        result.boundsMax = normalized.boundsMax;
                        LOG_I(
                            "Scene3DAsyncLoader: normalized GLTF radius {} -> {} (scale={})",
                            normalized.sourceRadius,
                            normalized.normalizedRadius,
                            normalized.uniformScale);
                    }
                }
            }

            if (autoPlayFirstAnimation && !gltfScene->animations.empty()) {
                auto animCtrl = result.sceneRoot->addComponent<GLTFAnimationController>();
                animCtrl->init(gltfScene, result.sceneRoot);
                animCtrl->play(gltfScene->animations[0].name);
                LOG_I("Scene3DAsyncLoader: playing GLTF animation '{}'", gltfScene->animations[0].name);
            }

            if (onSceneBuilt) {
                onSceneBuilt(gltfScene, result.sceneRoot);
            }

            return result;
        },
        std::move(sceneOptions));
}

void Scene3DAsyncLoader::cancel() {
    std::lock_guard<std::mutex> lock(m_mutex);
    const bool hadActiveLoad = m_loading || m_ready;
    ++m_generation;
    m_loading = false;
    m_ready = false;
    m_status = hadActiveLoad ? Status::Cancelled : Status::Idle;
    m_pendingPayload.reset();
    m_pendingError.clear();
    m_mainThreadApply = {};
    m_activeOptions = {};
    detachFrameBeginObserver();
}

bool Scene3DAsyncLoader::isLoading() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_loading;
}

Scene3DAsyncLoader::Status Scene3DAsyncLoader::getStatus() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_status;
}

std::string Scene3DAsyncLoader::getLastError() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastError;
}

void Scene3DAsyncLoader::handleWorkerCompletion(uint64_t generation,
                                                WorkerPayload payload,
                                                const std::string& error) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (generation != m_generation || !m_loading) {
            return;
        }
        m_pendingPayload = std::move(payload);
        m_pendingError = error;
        m_ready = true;
    }

    if (auto scene3DView = m_scene3DView.lock()) {
        scene3DView->requestRender("Scene3DAsyncLoader::handleWorkerCompletion");
    }
}

void Scene3DAsyncLoader::pumpPendingResult() {
    WorkerPayload payload;
    std::string error;
    MainThreadApply mainThreadApply;
    Scene3DAsyncLoadOptions activeOptions;
    uint64_t generation = 0;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_ready) {
            return;
        }

        generation = m_generation;
        payload = std::move(m_pendingPayload);
        error = std::move(m_pendingError);
        mainThreadApply = m_mainThreadApply;
        activeOptions = m_activeOptions;
        m_ready = false;
    }

    auto fail = [&](const std::string& failure) {
        std::function<void(const std::string&)> onError;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (generation != m_generation) {
                return;
            }
            m_loading = false;
            m_ready = false;
            m_status = Status::Failed;
            m_lastError = failure;
            m_pendingPayload.reset();
            m_pendingError.clear();
            m_mainThreadApply = {};
            m_activeOptions = {};
            onError = activeOptions.onError;
        }

        detachFrameBeginObserver();

        if (!activeOptions.debugLabel.empty()) {
            LOG_E("Scene3DAsyncLoader [{}]: {}", activeOptions.debugLabel, failure);
        } else {
            LOG_E("Scene3DAsyncLoader: {}", failure);
        }

        if (onError) onError(failure);
    };

    auto succeed = [&](const Scene3DAsyncLoadResult& result) {
        std::function<void(const Scene3DAsyncLoadResult&)> onLoaded;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (generation != m_generation) {
                return;
            }
            m_loading = false;
            m_ready = false;
            m_status = Status::Succeeded;
            m_lastError.clear();
            m_pendingPayload.reset();
            m_pendingError.clear();
            m_mainThreadApply = {};
            m_activeOptions = {};
            onLoaded = activeOptions.onLoaded;
        }

        detachFrameBeginObserver();

        if (!activeOptions.debugLabel.empty()) {
            LOG_I("Scene3DAsyncLoader [{}]: scene applied", activeOptions.debugLabel);
        } else {
            LOG_I("Scene3DAsyncLoader: scene applied");
        }

        if (onLoaded) onLoaded(result);
    };

    if (!error.empty()) {
        fail(error);
        return;
    }

    if (!payload) {
        fail("Background loader returned an empty payload");
        return;
    }

    auto scene3DView = m_scene3DView.lock();
    if (!scene3DView) {
        fail("Scene3DView was destroyed before the async scene finished loading");
        return;
    }

    if (!mainThreadApply) {
        fail("Missing main-thread apply callback");
        return;
    }

    Scene3DAsyncLoadResult result = mainThreadApply(payload);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (generation != m_generation || !m_loading) {
            return;
        }
    }

    if (!result.error.empty()) {
        fail(result.error);
        return;
    }

    if (!result.sceneRoot) {
        fail("Main-thread scene build returned no scene root");
        return;
    }

    if (result.hasBounds && activeOptions.cameraFit.enabled) {
        scene3DView->setSceneRoot(result.sceneRoot, result.boundsMin, result.boundsMax, activeOptions.cameraFit);
    } else {
        scene3DView->setSceneRoot(result.sceneRoot);
    }
    scene3DView->requestRender("Scene3DAsyncLoader::pumpPendingResult");

    succeed(result);
}
} // namespace morrow
