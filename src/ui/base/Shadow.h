//
// Created by 0060328 on 25-10-14.
//

#ifndef SHADOW_H
#define SHADOW_H
#include "Component.h"
#include "Material.h"
#include "MeshRenderer.h"

namespace morrow {

class Shadow : public Component{
public:
    Shadow();

    void update(FrameStateSharedPtr frameState) override;

    // 设置阴影偏移
    void setShadowOffset(const Vector2& offset);

    // 设置阴影颜色
    void setShadowColor(const Vector4& color);

private:
    Vector2 m_shadowOffsetInput = Vector2(5.0f, 5.0f);
    Vector2 m_shadowOffsetRender = Vector2(5.0f, -5.0f);
    Vector4 m_shadowColor = Vector4(0.0f, 0.0f, 0.0f, 0.5f);
    MaterialSharedPtr m_shadowMaterial;
};

} // morrow

#endif //SHADOW_H
