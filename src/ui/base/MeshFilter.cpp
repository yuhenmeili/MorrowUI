//
// Created by 0060328 on 25-9-25.
//

#include "MeshFilter.h"
#include "Component.inl"
#include "Transform.h"

namespace morrow {
MeshFilter::MeshFilter() {
    m_mesh = Mesh::createQuad(1.0f, 1.0f);
};

void MeshFilter::awake() {
    auto transform = getComponent<Transform>();
    transform->addSizeChangeListener([this, transform]() {
        const auto size = transform->getSize();
        const float halfW = size.x * 0.5f;
        const float halfH = size.y * 0.5f;

        std::vector<Vector3> vertices = {
            Vector3(-halfW, halfH, 0.0f),
            Vector3(halfW, halfH, 0.0f),
            Vector3(halfW, -halfH, 0.0f),
            Vector3(-halfW, -halfH, 0.0f),
        };
        m_mesh->setVertices(vertices);
    });
}

void MeshFilter::setUVData(const Vector2& uv0, const Vector2& uv1) {
    setUVData(uv0.x, uv0.y, uv1.x, uv1.y);
}

void MeshFilter::setUVData(float x0, float y0, float x1, float y1) {
    if (m_uv0.x != x0 || m_uv0.y != y0 || m_uv1.x != x1 || m_uv1.y != y1) {
        m_uv0.set(x0, y0);
        m_uv1.set(x1, y1);
        std::vector<Vector2> uvs = {
            m_uv0,
            Vector2(m_uv1.x, m_uv0.y),
            m_uv1,
            Vector2(m_uv0.x, m_uv1.y)
        };
        m_mesh->setUVs(uvs);
    }
}

void MeshFilter::setColor(const Vector4& color) {
    setColor(color.x, color.y, color.z, color.w);
}

void MeshFilter::setColor(float r, float g, float b, float a) {
    m_color.set(r, g, b, a);
    std::vector<Vector4> colors = {
        m_color,
        m_color,
        m_color,
        m_color
    };
    m_mesh->setColors(colors);
}

void MeshFilter::setMesh(const MeshSharedPtr& mesh) {
    m_mesh = mesh;
}

MeshSharedPtr MeshFilter::getMesh() const {
    return m_mesh;
}
} // morrow
