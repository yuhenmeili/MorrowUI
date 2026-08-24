//
// Created by 0060328 on 25-10-14.
//

#include "Shadow.h"
#include "MeshFilter.h"
#include "Transform.h"
#include "GlobalObject.h"
#include "Component.inl"
#include "OrthographicCamera.h"
#include "BatchManager.h"
#include <cmath>

namespace morrow {
Shadow::Shadow() {
    m_shadowMaterial = Material::create();
    m_shadowMaterial->setShader("shadow");
    // 几何偏移（对象空间，shader 内平移）与边缘渐隐偏移是两套独立参数
    m_shadowMaterial->setVector("shadowOffset", m_shadowOffsetRender);
    m_shadowMaterial->setVector("shadowOffsetFade", Vector2(std::abs(m_shadowOffsetInput.x), std::abs(m_shadowOffsetInput.y)));
    m_shadowMaterial->setVector("shadowColor", m_shadowColor);
    m_shadowMaterial->setFloat("alpha", 1.0f);
}

void Shadow::update(FrameStateSharedPtr frameState) {
    // 获取原始对象的MeshFilter
    auto meshFilter = getComponent<MeshFilter>();
    if (!meshFilter || !meshFilter->getMesh()) {
        return;
    }

    // 获取原始对象的Transform
    auto originalTransform = getComponent<Transform>();
    if (!originalTransform) {
        return;
    }

    // 通用渲染路径（renderStandardBatch）依赖原对象带 MeshRenderer 来取 VertexArray
    if (!getComponent<MeshRenderer>()) {
        return;
    }

    // 阴影尺寸跟随原始对象
    m_shadowMaterial->setVector("displaySize", originalTransform->getSize());

    // 作为 underlay 渲染项提交给 BatchManager，参与合批/排序/统计，
    // 且保证在原始对象之前绘制（阴影先画，被原对象覆盖）。
    frameState->batchManager->addRenderable(
        m_shadowMaterial, meshFilter, originalTransform, frameState->currentClip, true);
}

void Shadow::setShadowOffset(const Vector2& offset) {
    // External API uses screen-style offset: +x right, +y down.
    m_shadowOffsetInput = offset;
    m_shadowOffsetRender.set(offset.x, -offset.y);
    m_shadowMaterial->setVector("shadowOffset", m_shadowOffsetRender);
    m_shadowMaterial->setVector("shadowOffsetFade", Vector2(std::abs(offset.x), std::abs(offset.y)));
}

void Shadow::setShadowColor(const Vector4& color) {
    m_shadowColor = color;
    m_shadowMaterial->setVector("shadowColor", m_shadowColor);
}
} // morrow
