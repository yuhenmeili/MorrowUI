//
// Created by lance on 2026/9/29.
//

#include "SceneDisplayQuad.h"

#include <algorithm>
#include <utility>

#include "GlobalObject.h"
#include "RenderDeviceProxy.h"
#include "Vector2.h"
#include "Vector3.h"
#include "morrow/Camera.h"
#include "morrow/base/Transform.h"

namespace morrow {

SceneDisplayQuad::SceneDisplayQuad() = default;

SceneDisplayQuad::~SceneDisplayQuad() = default;

void SceneDisplayQuad::rebuild(float displayWidth, float displayHeight) {
    m_material = Material::create("scene3d_display");
    m_material->setBlendEnabled(false);

    // 颜色纹理每帧经 useTexture2D 绑定，材质上不设置静态纹理。
    const float halfW = std::max(displayWidth * 0.5f, 0.5f);
    const float halfH = std::max(displayHeight * 0.5f, 0.5f);

    // 两三角形四边形，局部原点为中心。UV 顶点顺序与 2D MeshFilter 一致，
    // V 翻转修正 GL render-target 纹理 (0,0) 在左下角的原点差异。
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

    auto vboData = std::make_shared<VBOData>();
    vboData->vertexCount = 4;
    vboData->indexCount = 6;
    vboData->drawMode = PrimitiveType::TRIANGLES;

    size_t posSize = 4 * sizeof(Vector3);
    size_t uvSize = 4 * sizeof(Vector2);
    vboData->vertexData.resize(posSize + uvSize);
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

    m_vboData = std::move(vboData);
    m_uploaded = false;
}

bool SceneDisplayQuad::draw(HwTexture2D colorTexture, const Camera* camera, Transform* transform) {
    RENDERINGTHREAD->useTexture2D(colorTexture, 0);

    if (transform && camera) {
        m_material->setMatrix4("projectionView", camera->getProjectionView());
        m_material->setMatrix4("model", transform->getWorldMatrix());
    }
    m_material->setInt("texture", 0);
    m_material->apply();

    auto shader = m_material->getShader();
    if (!shader) {
        return false;
    }
    if (!m_vbo.isValid()) {
        m_vbo = RENDERINGTHREAD->createVBO();
    }
    if (!m_uploaded && m_vboData) {
        RENDERINGTHREAD->updateVBO(shader, m_vbo, m_vboData);
        m_uploaded = true;
    }
    if (m_vbo.isValid() && m_uploaded) {
        RENDERINGTHREAD->drawVBO(m_vbo, 1);
        return true;
    }
    return false;
}

}  // namespace morrow
