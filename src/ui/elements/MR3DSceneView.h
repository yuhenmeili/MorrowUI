//
// Created by lance on 2026/3/31.
//

#ifndef MORROW_GUI_SCENE3DVIEW_H
#define MORROW_GUI_SCENE3DVIEW_H

#include <memory>
#include <vector>
#include "Material.h"
#include "OffscreenRenderTarget.h"
#include "OrbitCamera.h"
#include "Scene3DPassContext.h"
#include "controllers/OrbitController.h"
#include "base/UIWidget.h"
#include "base/SceneNode.h"

namespace morrow {
struct Scene3DCameraFitOptions {
    bool enabled = false;
    float paddingScale = 1.2f;
    float minRadius = 0.5f;
};

class MR3DSceneView : public UIWidget {
public:
    static std::shared_ptr<MR3DSceneView> create(int32_t fboW = 1280,
                                                 int32_t fboH = 720);

    ~MR3DSceneView() override;

    /// Replace the 3D scene subtree rendered by this view.
    void setSceneRoot(std::shared_ptr<SceneNode> sceneRoot);

    /// Replace the 3D scene subtree and optionally auto-fit the orbit camera.
    void setSceneRoot(std::shared_ptr<SceneNode> sceneRoot,
                      const Vector3& boundsMin,
                      const Vector3& boundsMax,
                      const Scene3DCameraFitOptions& fitOptions = {});

    const std::shared_ptr<SceneNode>& getSceneRoot() const;

    /// Fit the orbit camera to a world-space axis-aligned bounding box.
    bool fitCameraToBounds(const Vector3& boundsMin,
                           const Vector3& boundsMax,
                           const Scene3DCameraFitOptions& fitOptions = {});

    /// Access OrbitCamera for scene view control.
    OrbitCamera* getOrbitCamera() const;

    /// Access the interactive orbit controller bound to this scene view.
    OrbitController* getOrbitController() const;

    /// Compatibility accessor.
    OrbitCamera* getCamera() const;

    void setOrbitEnabled(bool enabled);

    bool isOrbitEnabled() const;

    void setSceneClearColor(const Vector4& clearColor);

    const Vector4& getSceneClearColor() const;

    void setSunLight(const Vector3& direction, const Vector3& color, float intensity);

    void setAmbientLight(const Vector3& color, float intensity);

    Scene3DLightingState& getLighting();

    const Scene3DLightingState& getLighting() const;

    void setIBL(const TextureSharedPtr& irradianceTexture,
                const TextureSharedPtr& specularTexture,
                const TextureSharedPtr& brdfLUTTexture,
                float rgbmRange,
                const std::vector<int32_t>& specularMipWidths,
                const std::vector<int32_t>& specularMipHeights,
                const std::vector<int32_t>& specularMipOffsetsY,
                float intensity = 1.0f);

    bool setIBLFromDirectory(const std::string& iblDirectory, float intensity = 1.0f);

    void clearIBL();

    Scene3DIBLState& getIBL();

    const Scene3DIBLState& getIBL() const;

    /// Resize the offscreen FBO.  Call when the view's display size changes.
    void resizeFBO(int32_t w, int32_t h);

    /// Widget update – issues 3D pass then composites into 2D pass.
    void update(FrameStateSharedPtr frameState) override;

private:
    MR3DSceneView();

    void syncToParentSize();

    /// Build the display quad in local widget space for the scene3d_display shader.
    void buildDisplayQuad();

    std::shared_ptr<OrbitCamera> m_orbitCamera;
    OrbitControllerSharedPtr m_orbitController;
    std::unique_ptr<OffscreenRenderTarget> m_renderTarget;
    std::shared_ptr<SceneNode> m_sceneRoot;
    MaterialSharedPtr m_displayMaterial;

    // Display quad GPU state (deferred VBO upload)
    VBODataSharedPtr m_displayQuadVBOData;
    VBO* m_quadVBO = nullptr;
    bool m_quadUploaded = false;

    Vector4 m_sceneClearColor = {0.22f, 0.23f, 0.31f, 1.0f};
    Scene3DPassContextSharedPtr m_scene3DPassContext;
    int32_t m_fboW = 1280, m_fboH = 720;
};
} // namespace morrow

#endif //MORROW_GUI_SCENE3DVIEW_H
