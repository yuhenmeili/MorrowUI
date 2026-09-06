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
    // 几何偏移（对象空间，shader 内平移）与 blur/spread/rounding 是独立参数
    m_shadowMaterial->setVector("shadowOffset", m_shadowOffsetRender);
    m_shadowMaterial->setFloat("shadowBlur", m_shadowBlur);
    m_shadowMaterial->setFloat("shadowSpread", m_shadowSpread);
    m_shadowMaterial->setFloat("shadowRounding", m_shadowRounding);
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
    auto meshRenderer = getComponent<MeshRenderer>();
    if (!meshRenderer) {
        return;
    }

    // 阴影尺寸跟随原始对象
    m_shadowMaterial->setVector("displaySize", originalTransform->getSize());

    // 圆角默认跟随属主材质的 rounding 参数（显式设置后不再跟随）
    if (m_shadowRoundingFollowOwner) {
        if (auto ownerMaterial = meshRenderer->getMaterial()) {
            m_shadowRounding = ownerMaterial->getFloatOr("rounding", 0.0f);
        }
    }
    m_shadowMaterial->setFloat("shadowRounding", m_shadowRounding);
    // 几何外扩容纳衰减区：blur + spread（shader 内再做下限保护）
    m_shadowMaterial->setFloat("shadowExpand", m_shadowBlur + m_shadowSpread);

    // 普通通道、以自然收集顺序绘制在属主之后；shader 的 outer-only 裁剪
    // 保证阴影不会画在属主上，因此可以投在先绘制的内容（卡片/面板）之上
    frameState->batchManager->addRenderable(
        m_shadowMaterial, meshFilter, originalTransform, frameState->currentClip, false);
}

void Shadow::setShadowOffset(const Vector2& offset) {
    // External API uses screen-style offset: +x right, +y down.
    m_shadowOffsetInput = offset;
    m_shadowOffsetRender.set(offset.x, -offset.y);
    m_shadowMaterial->setVector("shadowOffset", m_shadowOffsetRender);
}

void Shadow::setShadowColor(const Vector4& color) {
    m_shadowColor = color;
    m_shadowMaterial->setVector("shadowColor", m_shadowColor);
}

void Shadow::setShadowBlur(float blur) {
    m_shadowBlur = std::max(blur, 0.0f);
    m_shadowMaterial->setFloat("shadowBlur", m_shadowBlur);
}

void Shadow::setShadowSpread(float spread) {
    m_shadowSpread = spread;
    m_shadowMaterial->setFloat("shadowSpread", m_shadowSpread);
}

void Shadow::setShadowRounding(float rounding) {
    m_shadowRounding = std::max(rounding, 0.0f);
    m_shadowRoundingFollowOwner = false;
    m_shadowMaterial->setFloat("shadowRounding", m_shadowRounding);
}

void Shadow::setShadowRoundingFollowOwner(bool follow) {
    m_shadowRoundingFollowOwner = follow;
}
} // morrow
