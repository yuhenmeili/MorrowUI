//
// Created by lance on 2023/10/18.
//

#include "MeshRenderer.h"

#include "BatchManager.h"
#include "GlobalObject.h"
#include "Transform.h"
#include "Component.inl"
#include "OrthographicCamera.h"

namespace morrow {
MeshRenderer::MeshRenderer() {
    m_defaultVertexArray = std::make_shared<VertexArray>();
    m_material = Material::create();
}

MeshRenderer::~MeshRenderer() {
}

void MeshRenderer::update(FrameStateSharedPtr frameState) {
    auto meshFilter = getComponent<MeshFilter>();
    if (meshFilter) {
        auto transform = getComponent<Transform>();
        frameState->batchManager->addRenderable(m_material, meshFilter, transform);
    }
}

void MeshRenderer::setMaterial(const MaterialSharedPtr& material) {
    m_material = material;
}

MaterialSharedPtr MeshRenderer::getMaterial() const {
    return m_material;
}

VertexArraySharedPtr MeshRenderer::getVertexArray() const {
    return m_defaultVertexArray;
}
} // MORROWGUI
