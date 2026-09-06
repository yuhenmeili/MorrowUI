//
// Created by 0060328 on 25-10-14.
//

#ifndef SHADOW_H
#define SHADOW_H
#include "Component.h"
#include "Material.h"
#include "MeshRenderer.h"

namespace morrow {

// SDF 圆角软阴影组件（UI_SHADOW_DESIGN_PROPOSAL.md §3）：
//   复用属主的 MeshFilter/Transform，阴影形状由 shadow shader 的圆角矩形
//   SDF 解析求值（blur 高斯衰减 + spread 扩展 + rounding 圆角，圆角默认
//   跟随属主材质）。shader 内做 outer-only 裁剪（属主轮廓内部不绘制），
//   因此阴影以普通通道在属主之后绘制即可，不依赖 underlay 全局垫底，
//   可以投在先于属主绘制的内容（卡片/面板）之上。
class Shadow : public Component{
public:
    Shadow();

    void update(FrameStateSharedPtr frameState) override;

    // 设置阴影偏移（屏幕方向：+x 右，+y 下）
    void setShadowOffset(const Vector2& offset);

    // 设置阴影颜色（rgb + 不透明度）
    void setShadowColor(const Vector4& color);

    // 设置模糊半径（px）；默认 = 2 * max(offset)
    void setShadowBlur(float blur);

    // 设置阴影相对属主的扩展（正值扩大、负值收缩，px）
    void setShadowSpread(float spread);

    // 显式指定阴影圆角（px），此后不再跟随属主
    void setShadowRounding(float rounding);

    // 恢复圆角跟随属主材质的 rounding 参数（默认行为）
    void setShadowRoundingFollowOwner(bool follow);

private:
    // 默认：零偏移的黑色柔光（glow）——内区透明区正好藏在属主后面
    Vector2 m_shadowOffsetInput = Vector2(0.0f, 0.0f);
    Vector2 m_shadowOffsetRender = Vector2(0.0f, 0.0f);
    Vector4 m_shadowColor = Vector4(0.0f, 0.0f, 0.0f, 0.5f);
    float m_shadowBlur = 10.0f;
    float m_shadowSpread = 0.0f;
    float m_shadowRounding = 0.0f;
    bool m_shadowRoundingFollowOwner = true;
    MaterialSharedPtr m_shadowMaterial;
};

} // morrow

#endif //SHADOW_H
