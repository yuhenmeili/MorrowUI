//
// Created by 0060328 on 25-9-25.
//

#include "Mesh.h"
#include <cmath>

namespace morrow {

Mesh::Mesh() = default;

Mesh::~Mesh() = default;

void Mesh::setVertices(const std::vector<Vector3>& vertices) {
    m_vertices = vertices;
    ++m_revision;
}

const std::vector<Vector3>& Mesh::getVertices() const {
    return m_vertices;
}

void Mesh::setIndices(const std::vector<int16_t>& indices) {
    m_indices = indices;
    ++m_revision;
}

const std::vector<int16_t>& Mesh::getIndices() const {
    return m_indices;
}

void Mesh::setUVs(const std::vector<Vector2>& uvs) {
    m_uvs = uvs;
    ++m_revision;
}

const std::vector<Vector2>& Mesh::getUVs() const {
    return m_uvs;
}

void Mesh::setNormals(const std::vector<Vector3>& normals) {
    m_normals = normals;
    ++m_revision;
}

const std::vector<Vector3>& Mesh::getNormals() const {
    return m_normals;
}

void Mesh::setColors(const std::vector<Vector4>& colors) {
    m_colors = colors;
    ++m_revision;
}

const std::vector<Vector4>& Mesh::getColors() const {
    return m_colors;
}

size_t Mesh::getVertexCount() const {
    return m_vertices.size();
}

size_t Mesh::getIndexCount() const {
    return m_indices.size();
}

void Mesh::setDrawMode(PrimitiveType mode) {
    if (m_drawMode != mode) {
        m_drawMode = mode;
        ++m_revision;
    }
}

PrimitiveType Mesh::getDrawMode() const {
    return m_drawMode;
}

uint64_t Mesh::getRevision() const {
    return m_revision;
}

void Mesh::clear() {
    m_vertices.clear();
    m_colors.clear();
    m_uvs.clear();
    m_normals.clear();
    m_indices.clear();
    ++m_revision;
}

MeshSharedPtr Mesh::createQuad(float width, float height) {
    auto mesh = std::make_shared<Mesh>();
    auto halfWidth = width * 0.5f;
    auto halfHeight = height * 0.5f;

    std::vector<Vector3> vertices = {
        Vector3(-halfWidth, halfHeight, 0.0f),
        Vector3(halfWidth, halfHeight, 0.0f),
        Vector3(halfWidth, -halfHeight, 0.0f),
        Vector3(-halfWidth, -halfHeight, 0.0f),
    };
    
    std::vector<int16_t> indices = {
        0, 1, 2,  // 第一个三角形
        0, 2, 3   // 第二个三角形
    };
    
    std::vector<Vector2> uvs = {
        Vector2(0.0f, 0.0f),
        Vector2(1.0f, 0.0f),
        Vector2(1.0f, 1.0f),
        Vector2(0.0f, 1.0f),
    };
    
    std::vector<Vector3> normals = {
        Vector3(0.0f, 0.0f, 1.0f),
        Vector3(0.0f, 0.0f, 1.0f),
        Vector3(0.0f, 0.0f, 1.0f),
        Vector3(0.0f, 0.0f, 1.0f)
    };
    
    std::vector<Vector4> colors = {
        Vector4(1.0f, 1.0f, 1.0f, 1.0f),
        Vector4(1.0f, 1.0f, 1.0f, 1.0f),
        Vector4(1.0f, 1.0f, 1.0f, 1.0f),
        Vector4(1.0f, 1.0f, 1.0f, 1.0f)
    };
    
    mesh->setVertices(vertices);
    mesh->setIndices(indices);
    mesh->setUVs(uvs);
    mesh->setNormals(normals);
    mesh->setColors(colors);
    
    return mesh;
}

MeshSharedPtr Mesh::createCube(float size) {
    auto mesh = std::make_shared<Mesh>();
    
    float halfSize = size * 0.5f;
    
    // 创建立方体顶点 (24个顶点，每个面4个)
    std::vector<Vector3> vertices = {
        // 前面
        Vector3(-halfSize, -halfSize, halfSize),
        Vector3(halfSize, -halfSize, halfSize),
        Vector3(halfSize, halfSize, halfSize),
        Vector3(-halfSize, halfSize, halfSize),
        // 后面
        Vector3(halfSize, -halfSize, -halfSize),
        Vector3(-halfSize, -halfSize, -halfSize),
        Vector3(-halfSize, halfSize, -halfSize),
        Vector3(halfSize, halfSize, -halfSize),
        // 左面
        Vector3(-halfSize, -halfSize, -halfSize),
        Vector3(-halfSize, -halfSize, halfSize),
        Vector3(-halfSize, halfSize, halfSize),
        Vector3(-halfSize, halfSize, -halfSize),
        // 右面
        Vector3(halfSize, -halfSize, halfSize),
        Vector3(halfSize, -halfSize, -halfSize),
        Vector3(halfSize, halfSize, -halfSize),
        Vector3(halfSize, halfSize, halfSize),
        // 上面
        Vector3(-halfSize, halfSize, halfSize),
        Vector3(halfSize, halfSize, halfSize),
        Vector3(halfSize, halfSize, -halfSize),
        Vector3(-halfSize, halfSize, -halfSize),
        // 下面
        Vector3(-halfSize, -halfSize, -halfSize),
        Vector3(halfSize, -halfSize, -halfSize),
        Vector3(halfSize, -halfSize, halfSize),
        Vector3(-halfSize, -halfSize, halfSize)
    };
    
    // 索引 (36个索引，每个面2个三角形)
    std::vector<int16_t> indices = {
        0, 1, 2, 0, 2, 3,       // 前面
        4, 5, 6, 4, 6, 7,       // 后面
        8, 9, 10, 8, 10, 11,    // 左面
        12, 13, 14, 12, 14, 15, // 右面
        16, 17, 18, 16, 18, 19, // 上面
        20, 21, 22, 20, 22, 23  // 下面
    };
    
    // UV坐标
    std::vector<Vector2> uvs = {
        // 前面
        Vector2(0.0f, 0.0f), Vector2(1.0f, 0.0f), Vector2(1.0f, 1.0f), Vector2(0.0f, 1.0f),
        // 后面
        Vector2(0.0f, 0.0f), Vector2(1.0f, 0.0f), Vector2(1.0f, 1.0f), Vector2(0.0f, 1.0f),
        // 左面
        Vector2(0.0f, 0.0f), Vector2(1.0f, 0.0f), Vector2(1.0f, 1.0f), Vector2(0.0f, 1.0f),
        // 右面
        Vector2(0.0f, 0.0f), Vector2(1.0f, 0.0f), Vector2(1.0f, 1.0f), Vector2(0.0f, 1.0f),
        // 上面
        Vector2(0.0f, 0.0f), Vector2(1.0f, 0.0f), Vector2(1.0f, 1.0f), Vector2(0.0f, 1.0f),
        // 下面
        Vector2(0.0f, 0.0f), Vector2(1.0f, 0.0f), Vector2(1.0f, 1.0f), Vector2(0.0f, 1.0f)
    };
    
    // 法线
    std::vector<Vector3> normals;
    for (int i = 0; i < 4; i++) normals.push_back(Vector3(0.0f, 0.0f, 1.0f));
    for (int i = 0; i < 4; i++) normals.push_back(Vector3(0.0f, 0.0f, -1.0f));
    for (int i = 0; i < 4; i++) normals.push_back(Vector3(-1.0f, 0.0f, 0.0f));
    for (int i = 0; i < 4; i++) normals.push_back(Vector3(1.0f, 0.0f, 0.0f));
    for (int i = 0; i < 4; i++) normals.push_back(Vector3(0.0f, 1.0f, 0.0f));
    for (int i = 0; i < 4; i++) normals.push_back(Vector3(0.0f, -1.0f, 0.0f));
    
    // 颜色
    std::vector<Vector4> colors(24, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    
    mesh->setVertices(vertices);
    mesh->setIndices(indices);
    mesh->setUVs(uvs);
    mesh->setNormals(normals);
    mesh->setColors(colors);
    
    return mesh;
}

} // morrow
