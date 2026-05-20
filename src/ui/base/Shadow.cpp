//
// Created by 0060328 on 25-10-14.
//

#include "Shadow.h"
#include "MeshFilter.h"
#include "Transform.h"
#include "GlobalObject.h"
#include "Component.inl"
#include "OrthographicCamera.h"
#include <cmath>

namespace morrow {
Shadow::Shadow() {
    m_defaultVertexArray = std::make_shared<VertexArray>();
    m_shadowMaterial = Material::create();
    m_shadowMaterial->setShader("shadow");
    m_shadowMaterial->setVector("shadowOffset", Vector2(std::abs(m_shadowOffsetInput.x), std::abs(m_shadowOffsetInput.y)));
    m_shadowMaterial->setVector("shadowColor", m_shadowColor);
    m_shadowMaterial->setFloat("alpha", 1.0f);
    // m_shadowMaterial->setBlendEnabled(true);
    // m_shadowMaterial->setBlendFunc(1, 0x303); // GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
    // auto transform = getGameObject()->getComponent<Transform>();
    // transform->addSizeChangeListener([this, transform]() {
    //     m_shadowMaterial->setVector("displaySize", transform->getSize());
    // });
}

void Shadow::update(FrameStateSharedPtr frameState) {
    // 获取原始对象的MeshFilter
    auto meshFilter = getComponent<MeshFilter>();
    if (!meshFilter) {
        return;
    }

    auto mesh = meshFilter->getMesh();
    if (!mesh) {
        return;
    }

    // 获取原始对象的Transform
    auto originalTransform = getComponent<Transform>();
    if (!originalTransform) {
        return;
    }

    // 创建阴影的变换矩阵（带偏移）
    Matrix4 shadowModelMatrix = originalTransform->getWorldMatrix();

    // 应用阴影偏移
    Matrix4 shadowOffsetMatrix;
    shadowOffsetMatrix.makeTranslation(m_shadowOffsetRender.x, m_shadowOffsetRender.y, 0.0f);
    shadowModelMatrix = shadowModelMatrix * shadowOffsetMatrix;

    // 计算阴影的MVP矩阵
    Matrix4 viewProjectionMatrix = frameState->camera->getProjectionView();
    Matrix4 mvpMatrix = viewProjectionMatrix * shadowModelMatrix;

    // 设置阴影材质参数并渲染
    m_shadowMaterial->setMatrix4("mvp", mvpMatrix);
    m_shadowMaterial->setVector("displaySize", originalTransform->getSize());
    m_shadowMaterial->apply();

    // 渲染阴影
    m_defaultVertexArray->updateFromMeshes(frameState, m_shadowMaterial->getShader(), {meshFilter});
    m_defaultVertexArray->draw(frameState);
}

void Shadow::setShadowOffset(const Vector2& offset) {
    // External API uses screen-style offset: +x right, +y down.
    m_shadowOffsetInput = offset;
    m_shadowOffsetRender.set(offset.x, -offset.y);
    m_shadowMaterial->setVector("shadowOffset", Vector2(std::abs(offset.x), std::abs(offset.y)));
}

void Shadow::setShadowColor(const Vector4& color) {
    m_shadowColor = color;
    m_shadowMaterial->setVector("shadowColor", m_shadowColor);
}
} // morrow
