//
// Created by lance on 2026/3/31.
//

#ifndef MORROW_GUI_SCENE3DVIEW_H
#define MORROW_GUI_SCENE3DVIEW_H

#include <memory>
#include <vector>
#include <cstdint>
#include "Material.h"
#include "OffscreenRenderTarget.h"
#include "OrbitCamera.h"
#include "Scene3DPassContext.h"
#include "controllers/OrbitController.h"
#include "base/UIWidget.h"
#include "base/SceneNode.h"

namespace morrow {
/// 3D 场景相机自动适配参数。
struct Scene3DCameraFitOptions {
    /// 是否在设置场景根节点时自动适配相机。
    bool enabled = false;
    /// 包围盒外额外保留的缩放边距。
    float paddingScale = 1.2f;
    /// 自动适配时允许的最小相机半径。
    float minRadius = 0.5f;
};

/// 将 3D 场景离屏渲染并合成到 UI 中的视图组件。
class MR3DSceneView : public UIWidget {
public:
    /// 创建 3D 场景视图，并指定离屏帧缓冲区尺寸。
    static std::shared_ptr<MR3DSceneView> create(int32_t fboW = 1280,
                                                 int32_t fboH = 720);

    /// 销毁 3D 场景视图及其离屏渲染资源。
    ~MR3DSceneView() override;

    /// 替换当前视图渲染的 3D 场景根节点。
    /// Replace the 3D scene subtree rendered by this view.
    void setSceneRoot(std::shared_ptr<SceneNode> sceneRoot);

    /// 替换 3D 场景根节点，并按需根据包围盒自动适配环绕相机。
    /// Replace the 3D scene subtree and optionally auto-fit the orbit camera.
    void setSceneRoot(std::shared_ptr<SceneNode> sceneRoot,
                      const Vector3& boundsMin,
                      const Vector3& boundsMax,
                      const Scene3DCameraFitOptions& fitOptions = {});

    /// 获取当前渲染的 3D 场景根节点。
    const std::shared_ptr<SceneNode>& getSceneRoot() const;

    /// 根据世界空间轴对齐包围盒调整环绕相机。
    /// Fit the orbit camera to a world-space axis-aligned bounding box.
    bool fitCameraToBounds(const Vector3& boundsMin,
                           const Vector3& boundsMax,
                           const Scene3DCameraFitOptions& fitOptions = {});

    /// 获取用于控制场景视角的环绕相机。
    /// Access OrbitCamera for scene view control.
    OrbitCamera* getOrbitCamera() const;

    /// 获取绑定到当前场景视图的交互式环绕控制器。
    /// Access the interactive orbit controller bound to this scene view.
    OrbitController* getOrbitController() const;

    /// 获取环绕相机的兼容接口。
    /// Compatibility accessor.
    OrbitCamera* getCamera() const;

    /// 设置是否启用环绕相机交互。
    void setOrbitEnabled(bool enabled);

    /// 获取环绕相机交互是否启用。
    bool isOrbitEnabled() const;

    /// 设置 3D 场景离屏渲染的清屏颜色。
    void setSceneClearColor(const Vector4& clearColor);

    /// 获取 3D 场景离屏渲染的清屏颜色。
    const Vector4& getSceneClearColor() const;

    /// 设置场景主方向光的方向、颜色和强度。
    void setSunLight(const Vector3& direction, const Vector3& color, float intensity);

    /// 设置场景环境光的颜色和强度。
    void setAmbientLight(const Vector3& color, float intensity);

    /// 获取可修改的场景光照状态。
    Scene3DLightingState& getLighting();

    /// 获取只读场景光照状态。
    const Scene3DLightingState& getLighting() const;

    /// 设置基于图像的光照纹理、RGBM 范围和镜面反射 mip 布局。
    void setIBL(const TextureSharedPtr& irradianceTexture,
                const TextureSharedPtr& specularTexture,
                const TextureSharedPtr& brdfLUTTexture,
                float rgbmRange,
                const std::vector<int32_t>& specularMipWidths,
                const std::vector<int32_t>& specularMipHeights,
                const std::vector<int32_t>& specularMipOffsetsY,
                float intensity = 1.0f);

    /// 从指定目录加载并设置基于图像的光照资源。
    bool setIBLFromDirectory(const std::string& iblDirectory, float intensity = 1.0f);

    /// 清除当前基于图像的光照配置。
    void clearIBL();

    /// 获取可修改的基于图像光照状态。
    Scene3DIBLState& getIBL();

    /// 获取只读的基于图像光照状态。
    const Scene3DIBLState& getIBL() const;

    /// 调整离屏帧缓冲区尺寸；视图显示尺寸变化后应调用此方法。
    /// Resize the offscreen FBO.  Call when the view's display size changes.
    void resizeFBO(int32_t w, int32_t h);

    /// 使缓存的 3D 渲染结果失效，并在下一帧重新生成。
    /// Force the cached 3D pass to be regenerated on the next render.
    void invalidateSceneRender();

    /// 每帧执行 3D 离屏渲染，并将结果合成到 2D UI 渲染通道。
    /// Widget update – issues 3D pass then composites into 2D pass.
    void update(FrameStateSharedPtr frameState) override;

private:
    MR3DSceneView();

    void syncToParentSize();

    /// Build the display quad in local widget space for the scene3d_display shader.
    void buildDisplayQuad();

    uint64_t computeSceneRenderSignature() const;

    bool hasContinuousSceneUpdate() const;

    std::shared_ptr<OrbitCamera> m_orbitCamera;
    OrbitControllerSharedPtr m_orbitController;
    std::unique_ptr<OffscreenRenderTarget> m_renderTarget;
    std::shared_ptr<SceneNode> m_sceneRoot;
    MaterialSharedPtr m_displayMaterial;

    // Display quad GPU state (deferred VBO upload)
    VBODataSharedPtr m_displayQuadVBOData;
    HwVBO m_quadVBO{0};
    bool m_quadUploaded = false;

    Vector4 m_sceneClearColor = {0.22f, 0.23f, 0.31f, 1.0f};
    Scene3DPassContextSharedPtr m_scene3DPassContext;
    FrameStateSharedPtr m_local3DFrameState;
    int32_t m_fboW = 1280, m_fboH = 720;
    uint64_t m_lastSceneRenderSignature = 0;
    bool m_sceneRenderDirty = true;
    bool m_hasRenderedScene = false;
};
} // namespace morrow

#endif //MORROW_GUI_SCENE3DVIEW_H
