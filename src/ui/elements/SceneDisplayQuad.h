// SceneDisplayQuad.h — MR3DSceneView 离屏结果合成四边形（引擎内部工具）。
// 把 VBOData 构建与延迟上传等机械细节收进实现区，MR3DSceneView 公共头因此
// 不暴露 GpuTypes 类型。不对外。
#pragma once

#include <memory>

#include "GpuTypes.h"
#include "morrow/Material.h"
#include "morrow/ResourceHandle.h"

namespace morrow {

class Camera;
class Transform;

/// 用 scene3d_display 材质把 3D 离屏颜色纹理画到 2D 通道的显示四边形。
class SceneDisplayQuad {
public:
    SceneDisplayQuad();
    ~SceneDisplayQuad();

    SceneDisplayQuad(const SceneDisplayQuad&) = delete;
    SceneDisplayQuad& operator=(const SceneDisplayQuad&) = delete;

    /// 按显示尺寸（px）重建材质与四边形几何；尺寸变化后调用。
    void rebuild(float displayWidth, float displayHeight);

    /// 绑定颜色纹理并绘制。camera/transform 为空时跳过矩阵 uniform 更新
    /// （保持既有合成语义）。返回是否实际发出绘制（调用方累计 drawCallCount）。
    bool draw(HwTexture2D colorTexture, const Camera* camera, Transform* transform);

private:
    MaterialSharedPtr m_material;
    VBODataSharedPtr m_vboData;
    HwVBO m_vbo{0};
    bool m_uploaded = false;
};

}  // namespace morrow
