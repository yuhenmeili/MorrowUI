//
// Created by lance on 2023/10/18.
//

#ifndef MORROW_MESHRENDERER_H_
#define MORROW_MESHRENDERER_H_

#include "Vector2.h"
#include "morrow/base/Widget.h"
#include "morrow/Material.h"
#include "morrow/base/MeshFilter.h"

namespace morrow {
class VertexArray;
using VertexArraySharedPtr = std::shared_ptr<VertexArray>;
class BatchManager;

class MeshRenderer : public Component {
public:
    MeshRenderer();

    ~MeshRenderer() override;

    void update(FrameStateSharedPtr frameState) override;

    void setMaterial(const MaterialSharedPtr& material);

    MaterialSharedPtr getMaterial() const;

private:
    // 引擎内部批处理通道：VertexArray 是内部类型，签名不进公共面。
    friend class BatchManager;
    VertexArraySharedPtr getVertexArray() const;

    MaterialSharedPtr m_material;
    VertexArraySharedPtr m_defaultVertexArray;
};

using MeshRendererSharedPtr = std::shared_ptr<MeshRenderer>;
} // MORROWGUI

#endif //MORROW_MESHRENDERER_H_
