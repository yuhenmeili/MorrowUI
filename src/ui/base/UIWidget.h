//
// Created by lance on 2025/10/3.
//

#ifndef MORROW_GUI_UIWIDGET_H
#define MORROW_GUI_UIWIDGET_H
#include "base/Widget.h"
#include "MeshFilter.h"
#include "MeshRenderer.h"
#include "Material.h"
#include "core/GlobalDefine.h"

namespace morrow {
class Transform;

class UIWidget : public Widget {
public:
    explicit UIWidget(bool createRenderComponents = true);

    std::shared_ptr<Transform> getTransform();

    std::shared_ptr<Transform> getTransform() const;

    Math::Rect getScreenSpaceAABB() const;

    void setAlpha(float alpha);

    void setUVData(const Vector2& uv0, const Vector2& uv1);

    virtual void initialize();

protected:
    MeshFilterSharedPtr m_meshFilter;
    MeshRendererSharedPtr m_meshRenderer;
    MaterialSharedPtr m_material;
    float m_alpha = 1.0f;
};
} // morrow

#endif //MORROW_GUI_UIWIDGET_H
