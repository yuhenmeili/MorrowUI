//
// Created by lance on 2026/3/31.
//

#include "MR3DSceneView.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>

#include "GlobalObject.h"
#include "GlobalTools.h"
#include "OrthographicCamera.h"
#include "RenderDeviceProxy.h"
#include "Scene3DIBLLoader.h"
#include "Scene3DUBO.h"
#include "base/MeshRenderer3D.h"
#include "base/Transform.h"
#include "base/Transform3D.h"

namespace morrow {
namespace {
template <typename T>
std::shared_ptr<UBOData> makeUBOData(const T& payload) {
    auto* pool = RENDERINGTHREAD->getUBODataRecyclePool();
    auto uboData = pool ? pool->acquire() : std::make_shared<UBOData>();
    uboData->size = static_cast<uint32_t>(sizeof(T));
    uboData->data.resize(sizeof(T));
    std::memcpy(uboData->data.data(), &payload, sizeof(T));
    return uboData;
}

void hashCombine(uint64_t& seed, uint64_t value) {
    seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
}

void hashFloat(uint64_t& seed, float value) {
    uint32_t bits = 0;
    static_assert(sizeof(bits) == sizeof(value));
    std::memcpy(&bits, &value, sizeof(bits));
    hashCombine(seed, bits);
}

void hashVector3(uint64_t& seed, const Vector3& value) {
    hashFloat(seed, value.x);
    hashFloat(seed, value.y);
    hashFloat(seed, value.z);
}

void hashTexture(uint64_t& seed, const TextureSharedPtr& texture) {
    hashCombine(seed, static_cast<uint64_t>(reinterpret_cast<uintptr_t>(texture.get())));
    if (texture) {
        hashCombine(seed, texture->getRevision());
    }
}

template <typename T>
void hashVector(uint64_t& seed, const std::vector<T>& values) {
    hashCombine(seed, values.size());
    for (const auto value : values) {
        hashCombine(seed, static_cast<uint64_t>(value));
    }
}
}  // namespace
// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------
std::shared_ptr<MR3DSceneView> MR3DSceneView::create(int32_t fboW, int32_t fboH) {
    auto view = std::shared_ptr<MR3DSceneView>(new MR3DSceneView());
    view->setWidgetName("Scene3DView");
    view->m_fboW = fboW;
    view->m_fboH = fboH;

    // Orbit camera with sensible defaults for model viewing
    view->m_orbitCamera = std::make_shared<OrbitCamera>(45.0f, float(fboW) / float(fboH), 0.1f, 2000.0f);
    view->m_orbitCamera->setPosition(0.0f, 0.0f, 5.0f);
    view->m_orbitCamera->lookAt(0.0f, 0.0f, 0.0f);
    view->m_orbitCamera->update(0.0f, 0.0f, float(fboW), float(fboH));
    view->m_orbitController = std::make_shared<OrbitController>(view);
    std::weak_ptr<MR3DSceneView> weakView = view;
    view->m_orbitCamera->setChangeCallback([weakView]() {
        if (auto strongView = weakView.lock()) {
            strongView->invalidateSceneRender();
        }
    });
    view->m_scene3DPassContext->camera = view->m_orbitCamera;
    view->m_scene3DPassContext->passWidth = fboW;
    view->m_scene3DPassContext->passHeight = fboH;

    if (auto transform = view->getTransform()) {
        transform->setSize(float(fboW), float(fboH));
        transform->addSizeChangeListener([weakView]() {
            if (auto strongView = weakView.lock()) {
                strongView->buildDisplayQuad();
                strongView->m_quadUploaded = false;
            }
        });
    }

    // Allocate the offscreen render target.
    view->m_renderTarget = std::make_unique<OffscreenRenderTarget>();
    view->m_renderTarget->create(fboW, fboH);

    // Build the 2D display quad that blits the FBO onto the UI layer.
    view->buildDisplayQuad();

    return view;
}

MR3DSceneView::MR3DSceneView() : UIWidget(false) {
    setWidgetType("Scene3DView");
    m_scene3DPassContext = std::make_shared<Scene3DPassContext>();
    m_local3DFrameState = std::make_shared<FrameState>();
}

MR3DSceneView::~MR3DSceneView() {
    if (m_renderTarget) {
        m_renderTarget->destroy();
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void MR3DSceneView::setSceneRoot(std::shared_ptr<SceneNode> sceneRoot) {
    m_sceneRoot = std::move(sceneRoot);
    invalidateSceneRender();
}

void MR3DSceneView::setSceneRoot(std::shared_ptr<SceneNode> sceneRoot, const Vector3& boundsMin, const Vector3& boundsMax, const Scene3DCameraFitOptions& fitOptions) {
    m_sceneRoot = std::move(sceneRoot);
    fitCameraToBounds(boundsMin, boundsMax, fitOptions);
    invalidateSceneRender();
}

const std::shared_ptr<SceneNode>& MR3DSceneView::getSceneRoot() const {
    return m_sceneRoot;
}

bool MR3DSceneView::fitCameraToBounds(const Vector3& boundsMin, const Vector3& boundsMax, const Scene3DCameraFitOptions& fitOptions) {
    if (!fitOptions.enabled || !m_orbitCamera)
        return false;

    const Vector3 sceneCenter = (boundsMin + boundsMax) * 0.5f;
    const Vector3 sceneExtent = boundsMax - boundsMin;
    const float sceneRadius = std::max(sceneExtent.length() * 0.5f, fitOptions.minRadius);

    const float aspect = (m_fboH > 0) ? (float(m_fboW) / float(m_fboH)) : 1.0f;
    const float fovYRadians = m_orbitCamera->getFov() * global_tools::kPi / 180.0f;
    const float halfFovY = fovYRadians * 0.5f;
    const float halfFovX = std::atan(std::tan(halfFovY) * std::max(aspect, 0.001f));
    const float limitingHalfFov = std::max(std::min(halfFovY, halfFovX), 0.001f);

    const float fitDistance = sceneRadius / std::sin(limitingHalfFov);
    const float finalDistance = std::max(fitDistance * fitOptions.paddingScale, fitOptions.minRadius);

    m_orbitCamera->lookAt(sceneCenter);
    m_orbitCamera->setDistance(finalDistance);
    invalidateSceneRender();

    LOG_I("Scene3DView fit camera: min=({}, {}, {}), max=({}, {}, {}), radius={}, distance={}", boundsMin.x, boundsMin.y, boundsMin.z, boundsMax.x, boundsMax.y, boundsMax.z, sceneRadius,
          finalDistance);
    return true;
}

OrbitCamera* MR3DSceneView::getOrbitCamera() const {
    return m_orbitCamera.get();
}

OrbitController* MR3DSceneView::getOrbitController() const {
    return m_orbitController.get();
}

OrbitCamera* MR3DSceneView::getCamera() const {
    return m_orbitCamera.get();
}

void MR3DSceneView::setOrbitEnabled(bool enabled) {
    if (m_orbitController) {
        m_orbitController->setEnabled(enabled);
    }
}

bool MR3DSceneView::isOrbitEnabled() const {
    return m_orbitController && m_orbitController->isEnabled();
}

void MR3DSceneView::setSceneClearColor(const Vector4& clearColor) {
    m_sceneClearColor = clearColor;
    invalidateSceneRender();
}

const Vector4& MR3DSceneView::getSceneClearColor() const {
    return m_sceneClearColor;
}

void MR3DSceneView::setSunLight(const Vector3& direction, const Vector3& color, float intensity) {
    m_scene3DPassContext->lighting.sunDirection = direction;
    m_scene3DPassContext->lighting.sunColor = color;
    m_scene3DPassContext->lighting.sunIntensity = intensity;
    invalidateSceneRender();
}

void MR3DSceneView::setAmbientLight(const Vector3& color, float intensity) {
    m_scene3DPassContext->lighting.ambientColor = color;
    m_scene3DPassContext->lighting.ambientIntensity = intensity;
    invalidateSceneRender();
}

Scene3DLightingState& MR3DSceneView::getLighting() {
    invalidateSceneRender();
    return m_scene3DPassContext->lighting;
}

const Scene3DLightingState& MR3DSceneView::getLighting() const {
    return m_scene3DPassContext->lighting;
}

void MR3DSceneView::setIBL(const TextureSharedPtr& irradianceTexture, const TextureSharedPtr& specularTexture, const TextureSharedPtr& brdfLUTTexture, float rgbmRange,
                           const std::vector<int32_t>& specularMipWidths, const std::vector<int32_t>& specularMipHeights, const std::vector<int32_t>& specularMipOffsetsY, float intensity) {
    m_scene3DPassContext->ibl.irradianceTexture = irradianceTexture;
    m_scene3DPassContext->ibl.specularTexture = specularTexture;
    m_scene3DPassContext->ibl.brdfLUTTexture = brdfLUTTexture;
    m_scene3DPassContext->ibl.rgbmRange = rgbmRange;
    m_scene3DPassContext->ibl.intensity = intensity;
    m_scene3DPassContext->ibl.specularMipWidths = specularMipWidths;
    m_scene3DPassContext->ibl.specularMipHeights = specularMipHeights;
    m_scene3DPassContext->ibl.specularMipOffsetsY = specularMipOffsetsY;
    invalidateSceneRender();
}

bool MR3DSceneView::setIBLFromDirectory(const std::string& iblDirectory, float intensity) {
    Scene3DIBLState ibl;
    if (!Scene3DIBLLoader::loadFromDirectory(iblDirectory, ibl, intensity)) {
        return false;
    }

    setIBL(ibl.irradianceTexture, ibl.specularTexture, ibl.brdfLUTTexture, ibl.rgbmRange, ibl.specularMipWidths, ibl.specularMipHeights, ibl.specularMipOffsetsY, ibl.intensity);
    return true;
}

void MR3DSceneView::clearIBL() {
    m_scene3DPassContext->ibl = {};
    invalidateSceneRender();
}

Scene3DIBLState& MR3DSceneView::getIBL() {
    invalidateSceneRender();
    return m_scene3DPassContext->ibl;
}

const Scene3DIBLState& MR3DSceneView::getIBL() const {
    return m_scene3DPassContext->ibl;
}

void MR3DSceneView::resizeFBO(int32_t w, int32_t h) {
    if (w <= 0 || h <= 0) {
        return;
    }

    if (m_renderTarget) {
        m_renderTarget->resize(w, h);
    }
    m_fboW = w;
    m_fboH = h;
    if (m_scene3DPassContext) {
        m_scene3DPassContext->passWidth = w;
        m_scene3DPassContext->passHeight = h;
    }
    m_orbitCamera->update(0.0f, 0.0f, float(w), float(h));
    if (auto transform = getTransform()) {
        transform->setSize(float(w), float(h));
    }
    invalidateSceneRender();
}

void MR3DSceneView::invalidateSceneRender() {
    m_sceneRenderDirty = true;
    requestRender("MR3DSceneView::invalidateSceneRender");
}

void MR3DSceneView::syncToParentSize() {
    auto transform = getTransform();
    if (!transform || !m_parent) {
        return;
    }

    auto parentTransform = m_parent->getComponent<Transform>();
    if (!parentTransform) {
        return;
    }

    const Vector3 parentSize = parentTransform->getSize();
    if (parentSize.x <= 0.0f || parentSize.y <= 0.0f) {
        return;
    }

    const auto targetW = static_cast<int32_t>(std::lround(parentSize.x));
    const auto targetH = static_cast<int32_t>(std::lround(parentSize.y));
    if (targetW != m_fboW || targetH != m_fboH) {
        resizeFBO(targetW, targetH);
    }
}

// ---------------------------------------------------------------------------
// Display quad
// ---------------------------------------------------------------------------

void MR3DSceneView::buildDisplayQuad() {
    // A simple textured quad in local widget space. The shader samples the FBO
    // texture with a y-flip to account for the OpenGL FBO origin difference.
    m_displayMaterial = Material::create("scene3d_display");
    m_displayMaterial->setBlendEnabled(false);

    // The colour texture will be bound each frame via useTexture2D before draw.
    // No static texture is set on the material; we bind the FBO texture manually.

    auto transform = getTransform();
    Vector3 displaySize = transform ? transform->getSize() : Vector3(float(m_fboW), float(m_fboH), 0.0f);
    const float halfW = std::max(displaySize.x * 0.5f, 0.5f);
    const float halfH = std::max(displaySize.y * 0.5f, 0.5f);

    // Quad VBOData: two triangles centered at the local origin.
    auto vboData = std::make_shared<VBOData>();
    vboData->vertexCount = 4;
    vboData->indexCount = 6;
    vboData->drawMode = PrimitiveType::TRIANGLES;

    // Positions: local widget-space quad centered around origin.
    // UV: keep the same vertex order as 2D MeshFilter, but flip V because
    // OpenGL render-target textures are sampled with (0,0) at the bottom-left.
    struct QuadVertex {
        float x, y, z;
        float u, v;
    };
    const QuadVertex verts[4] = {
        {-halfW, halfH, 0.0f, 0.0f, 1.0f},
        {halfW, halfH, 0.0f, 1.0f, 1.0f},
        {halfW, -halfH, 0.0f, 1.0f, 0.0f},
        {-halfW, -halfH, 0.0f, 0.0f, 0.0f},
    };
    const int16_t indices[6] = {0, 1, 2, 0, 2, 3};

    size_t posSize = 4 * sizeof(Vector3);
    size_t uvSize = 4 * sizeof(Vector2);
    vboData->vertexData.resize(posSize + uvSize);

    // Pack positions
    vboData->attributes.push_back({VertexAttributeType::Position, 0, sizeof(Vector3)});
    vboData->attributes.push_back({VertexAttributeType::UV, posSize, sizeof(Vector2)});

    for (int i = 0; i < 4; ++i) {
        auto* p = reinterpret_cast<Vector3*>(vboData->vertexData.data() + i * sizeof(Vector3));
        p->x = verts[i].x;
        p->y = verts[i].y;
        p->z = verts[i].z;
        auto* uv = reinterpret_cast<Vector2*>(vboData->vertexData.data() + posSize + i * sizeof(Vector2));
        uv->x = verts[i].u;
        uv->y = verts[i].v;
    }
    vboData->indices.assign(indices, indices + 6);

    // Store VBOData; the VBO will be created lazily in the first update() call.
    m_displayQuadVBOData = std::move(vboData);
}

uint64_t MR3DSceneView::computeSceneRenderSignature() const {
    uint64_t signature = 0xcbf29ce484222325ULL;
    hashCombine(signature, static_cast<uint64_t>(m_fboW));
    hashCombine(signature, static_cast<uint64_t>(m_fboH));
    hashFloat(signature, m_sceneClearColor.x);
    hashFloat(signature, m_sceneClearColor.y);
    hashFloat(signature, m_sceneClearColor.z);
    hashFloat(signature, m_sceneClearColor.w);

    if (m_orbitCamera) {
        hashVector3(signature, m_orbitCamera->getPosition());
        hashVector3(signature, m_orbitCamera->getDirection());
        hashVector3(signature, m_orbitCamera->getUp());
        hashVector3(signature, m_orbitCamera->getTarget());
    }

    if (m_scene3DPassContext) {
        const auto& lighting = m_scene3DPassContext->lighting;
        hashVector3(signature, lighting.sunDirection);
        hashVector3(signature, lighting.sunColor);
        hashFloat(signature, lighting.sunIntensity);
        hashVector3(signature, lighting.ambientColor);
        hashFloat(signature, lighting.ambientIntensity);

        const auto& ibl = m_scene3DPassContext->ibl;
        hashTexture(signature, ibl.irradianceTexture);
        hashTexture(signature, ibl.specularTexture);
        hashTexture(signature, ibl.brdfLUTTexture);
        hashFloat(signature, ibl.rgbmRange);
        hashFloat(signature, ibl.intensity);
        hashVector(signature, ibl.specularMipWidths);
        hashVector(signature, ibl.specularMipHeights);
        hashVector(signature, ibl.specularMipOffsetsY);
    }

    std::function<void(const std::shared_ptr<Widget>&)> hashNode = [&](const std::shared_ptr<Widget>& node) {
        if (!node) {
            hashCombine(signature, 0);
            return;
        }

        hashCombine(signature, static_cast<uint64_t>(reinterpret_cast<uintptr_t>(node.get())));
        const bool visible = node->getVisible();
        hashCombine(signature, visible ? 1U : 0U);
        if (!visible) {
            return;
        }
        hashCombine(signature, static_cast<uint64_t>(node->getDisplayLayer()));

        if (auto transform = node->getComponent<Transform3D>()) {
            hashCombine(signature, transform->getLocalVersion());
        }
        if (auto renderer = node->getComponent<MeshRenderer3D>()) {
            hashCombine(signature, renderer->isEnabled() ? 1U : 0U);
            hashCombine(signature, renderer->getRenderRevision());
        }

        hashCombine(signature, node->m_children.size());
        for (const auto& child : node->m_children) {
            hashNode(child);
        }
    };
    hashNode(m_sceneRoot);
    return signature;
}

bool MR3DSceneView::hasContinuousSceneUpdate() const {
    bool active = false;
    std::function<void(const std::shared_ptr<Widget>&)> visit = [&](const std::shared_ptr<Widget>& node) {
        if (!node || active || !node->getVisible()) {
            return;
        }
        if (const auto* components = node->getComponentManager(); components && components->requiresContinuousUpdate()) {
            active = true;
            return;
        }
        for (const auto& child : node->m_children) {
            visit(child);
        }
    };
    visit(m_sceneRoot);
    return active;
}

// ---------------------------------------------------------------------------
// Per-frame update
// ---------------------------------------------------------------------------

void MR3DSceneView::update(FrameStateSharedPtr frameState) {
    if (!m_renderTarget || !m_renderTarget->getRenderTarget())
        return;

    syncToParentSize();

    if (m_orbitController) {
        m_orbitController->update(frameState);
    }
    if (m_orbitCamera) {
        m_orbitCamera->update(0.0f, 0.0f, float(m_fboW), float(m_fboH));
    }

    const uint64_t sceneSignature = computeSceneRenderSignature();
    const bool continuousSceneUpdate = hasContinuousSceneUpdate();
    const bool renderScene = !m_hasRenderedScene || m_sceneRenderDirty || continuousSceneUpdate || sceneSignature != m_lastSceneRenderSignature;

    if (renderScene) {
        RENDERINGTHREAD->bindRenderTarget(m_renderTarget->getRenderTarget());
        RENDERINGTHREAD->setClearColor(m_sceneClearColor.x, m_sceneClearColor.y, m_sceneClearColor.z, m_sceneClearColor.w);
        RENDERINGTHREAD->clear();

        if (m_sceneRoot && m_orbitCamera) {
            if (!m_scene3DPassContext->frameUBO) {
                m_scene3DPassContext->frameUBO = RENDERINGTHREAD->createUBO();
            }

            *m_local3DFrameState = *frameState;

            m_scene3DPassContext->camera = m_orbitCamera;
            m_scene3DPassContext->passWidth = m_fboW;
            m_scene3DPassContext->passHeight = m_fboH;

            const Scene3DFrameUBO frameUboPayload = buildScene3DFrameUBO(*m_scene3DPassContext);
            RENDERINGTHREAD->updateUBO(m_scene3DPassContext->frameUBO, makeUBOData(frameUboPayload));

            m_local3DFrameState->perspectiveCamera = m_orbitCamera;
            m_local3DFrameState->batchManager = nullptr;
            m_local3DFrameState->scene3DPassContext = m_scene3DPassContext;
            m_local3DFrameState->scene3DLighting = m_scene3DPassContext->lighting;
            m_local3DFrameState->scene3DIBL = m_scene3DPassContext->ibl;
            m_local3DFrameState->scene3DFrameUBO = m_scene3DPassContext->frameUBO;
            m_local3DFrameState->drawCallCount = 0;

            m_sceneRoot->update(m_local3DFrameState);
            frameState->drawCallCount += m_local3DFrameState->drawCallCount;
        }

        RENDERINGTHREAD->unbindRenderTarget();

        const uint64_t renderedSceneSignature = computeSceneRenderSignature();
        const bool changedDuringScenePass = renderedSceneSignature != sceneSignature;
        m_lastSceneRenderSignature = changedDuringScenePass ? sceneSignature : renderedSceneSignature;
        m_sceneRenderDirty = changedDuringScenePass;
        m_hasRenderedScene = true;
        if (continuousSceneUpdate || changedDuringScenePass) {
            requestRender(changedDuringScenePass ? "MR3DSceneView::sceneChangedDuringPass" : "MR3DSceneView::continuousSceneUpdate");
        }
    }

    // 2D composite pass
    if (m_renderTarget->getColorTexture() && frameState->camera) {
        // Bind the FBO colour texture to slot 0 and draw the display quad
        RENDERINGTHREAD->useTexture2D(m_renderTarget->getColorTexture(), 0);

        if (auto transform = getTransform()) {
            m_displayMaterial->setMatrix4("projectionView", frameState->camera->getProjectionView());
            m_displayMaterial->setMatrix4("model", transform->getWorldMatrix());
        }
        m_displayMaterial->setInt("texture", 0);
        m_displayMaterial->apply();

        auto shader = m_displayMaterial->getShader();
        if (shader) {
            if (!m_quadVBO.isValid()) {
                m_quadVBO = RENDERINGTHREAD->createVBO();
            }
            if (!m_quadUploaded && m_displayQuadVBOData) {
                RENDERINGTHREAD->updateVBO(shader, m_quadVBO, m_displayQuadVBOData);
                m_quadUploaded = true;
            }
            if (m_quadVBO.isValid() && m_quadUploaded) {
                frameState->drawCallCount++;
                RENDERINGTHREAD->drawVBO(m_quadVBO, 1);
            }
        }
    }

    // Propagate normal 2D child updates (labels, buttons, etc. overlaid on 3D)
    Widget::update(frameState);
}
}  // namespace morrow
