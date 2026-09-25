//
// BackdropBlur — 毛玻璃 / acrylic 背景模糊组件（实现）。
// 设计说明见 BackdropBlur.h 与 docs/road_map/KAWASE_BACKDROP_BLUR_PROPOSAL.md。
//

#include "BackdropBlur.h"

#include "BackdropBlurManager.h"
#include "BatchManager.h"
#include "GlobalObject.h"
#include "base/Component.inl"
#include "base/MeshFilter.h"
#include "base/MeshRenderer.h"
#include "base/Transform.h"
#include "base/Widget.h"

#include <algorithm>

namespace morrow {

BackdropBlur::BackdropBlur() {
    // 降级材质只在关闭模糊的帧里被实际使用（Shadow 同款：组件自持、
    // 构造时机保证渲染设备已就绪）
    m_tintMaterial = Material::create();
    m_tintMaterial->setShader("backdrop_tint");
    m_tintMaterial->setVector("tintColor", m_tintColor);
    m_tintMaterial->setFloat("rounding", m_rounding);
    m_tintMaterial->setFloat("alpha", 1.0f);
}

void BackdropBlur::update(FrameStateSharedPtr frameState) {
    if (!frameState || !frameState->batchManager) {
        return;
    }
    auto transform = getComponent<Transform>();
    if (!transform) {
        return;
    }

    auto meshRenderer = getComponent<MeshRenderer>();
    MaterialSharedPtr ownerMaterial = meshRenderer ? meshRenderer->getMaterial() : nullptr;

    // 圆角默认跟随属主材质的 rounding 参数（显式设置后不再跟随）
    if (m_roundingFollowOwner) {
        m_rounding = ownerMaterial ? ownerMaterial->getFloatOr("rounding", 0.0f) : 0.0f;
    }
    const float ownerAlpha = ownerMaterial ? ownerMaterial->getFloatOr("alpha", 1.0f) : 1.0f;

    auto& manager = BackdropBlurManager::getInstance();
    if (manager.isEnabled() && (m_explicitLevel || m_blurRadius > 0.0f)) {
        // 启用：提交给共享模糊链。层级来源二选一：setBlurLevel 的连续层级
        //（半径动画双层插值）或半径量化（§5.2：≤12 / ≤40 / >40 px）。
        // tint.a 同时承担混色强度与面片透明度，再叠加属主自身 alpha，
        // 保持与普通 Widget 的 setAlpha 语义一致。clipRect 取当前裁剪栈
        //（滚动容器等，S3 裁剪专项）。
        const Vector3 size = transform->getSize();
        const float levelF = m_explicitLevel ? m_blurLevel : static_cast<float>(BackdropBlurManager::quantizeRadius(m_blurRadius));
        manager.submitQuad(transform->getWorldMatrix(), Vector2(size.x, size.y), m_rounding,
                           Vector4(m_tintColor.x, m_tintColor.y, m_tintColor.z, m_tintColor.w * ownerAlpha),
                           levelF, getGameObject()->getDisplayLayer(), frameState->currentClip);
        return;
    }

    // 降级（radius=0 或全局关闭）：tint 纯色半透明面板，普通通道自然顺序
    // 绘制在属主之后（u_mvp 由 renderStandardBatch 逐对象设置）
    auto meshFilter = getComponent<MeshFilter>();
    if (!meshFilter || !meshFilter->getMesh() || !meshRenderer) {
        return;
    }
    m_tintMaterial->setVector("displaySize", transform->getSize());
    m_tintMaterial->setFloat("rounding", m_rounding);
    m_tintMaterial->setFloat("alpha", ownerAlpha);
    frameState->batchManager->addRenderable(m_tintMaterial, meshFilter, transform, frameState->currentClip, false);
}

void BackdropBlur::setBlurRadius(float radius) {
    m_blurRadius = std::max(radius, 0.0f);
    m_explicitLevel = false;
}

void BackdropBlur::setBlurLevel(float level) {
    m_blurLevel = level;
    m_explicitLevel = true;
    // 层级动画不触碰材质 / Transform，按需渲染模式下需要显式唤醒渲染循环
    REQUESTRENDER;
}

void BackdropBlur::setTintColor(const Vector4& color) {
    m_tintColor = color;
    m_tintMaterial->setVector("tintColor", m_tintColor);
}

void BackdropBlur::setTintColor(float r, float g, float b, float a) {
    setTintColor(Vector4(r, g, b, a));
}

void BackdropBlur::setRounding(float rounding) {
    m_rounding = std::max(rounding, 0.0f);
    m_roundingFollowOwner = false;
    m_tintMaterial->setFloat("rounding", m_rounding);
}

void BackdropBlur::setRoundingFollowOwner(bool follow) {
    m_roundingFollowOwner = follow;
}

} // namespace morrow
