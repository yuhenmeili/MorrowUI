//
// Created by lance on 2023/10/18.
//

#ifndef MORROW_MESHRENDERER_H_
#define MORROW_MESHRENDERER_H_

#include "Vector2.h"
#include "Widget.h"
#include "Material.h"
#include "VertexArray.h"
#include "RenderDeviceProxyBase.h"
#include "MeshFilter.h"

namespace morrow {
class MeshRenderer : public Component {
public:
    MeshRenderer();

    ~MeshRenderer() override;

    void update(FrameStateSharedPtr frameState) override;

    void setMaterial(const MaterialSharedPtr& material);

    MaterialSharedPtr getMaterial() const;

    VertexArraySharedPtr getVertexArray() const;

private:
    MaterialSharedPtr m_material;
    VertexArraySharedPtr m_defaultVertexArray;
};

using MeshRendererSharedPtr = std::shared_ptr<MeshRenderer>;
} // MORROWGUI

#endif //MORROW_MESHRENDERER_H_
