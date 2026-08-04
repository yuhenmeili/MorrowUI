//
// Created by 0060328 on 25-9-25.
//

#ifndef MESH_H
#define MESH_H

#include <vector>
#include <memory>
#include <cstdint>

#include "DriverEnums.h"
#include "Vector3.h"
#include "Vector2.h"
#include "Vector4.h"

namespace morrow {
using namespace Math;

class Mesh {
public:
    // 构造函数
    Mesh();

    ~Mesh();

    // 顶点相关方法
    void setVertices(const std::vector<Vector3>& vertices);

    const std::vector<Vector3>& getVertices() const;

    // 索引相关方法
    void setIndices(const std::vector<int16_t>& indices);

    const std::vector<int16_t>& getIndices() const;

    // UV相关方法
    void setUVs(const std::vector<Vector2>& uvs);

    const std::vector<Vector2>& getUVs() const;

    // 法线相关方法
    void setNormals(const std::vector<Vector3>& normals);

    const std::vector<Vector3>& getNormals() const;

    // 顶点颜色相关方法
    void setColors(const std::vector<Vector4>& colors);

    const std::vector<Vector4>& getColors() const;

    // 简单的几何体创建方法
    static std::shared_ptr<Mesh> createQuad(float width, float height);

    static std::shared_ptr<Mesh> createCube(float size);

    // 获取顶点数量
    size_t getVertexCount() const;

    // 获取索引数量
    size_t getIndexCount() const;

    void setDrawMode(PrimitiveType mode);

    PrimitiveType getDrawMode() const;

    uint64_t getBatchCompatibilityHash() const;

    uint64_t getRevision() const;

    void clear();

private:
    void markBatchCompatibilityDirty();

    std::vector<Vector3> m_vertices;
    std::vector<int16_t> m_indices;
    std::vector<Vector2> m_uvs;
    std::vector<Vector3> m_normals;
    std::vector<Vector4> m_colors;
    PrimitiveType m_drawMode = PrimitiveType::TRIANGLES;
    uint64_t m_revision = 1;
    uint64_t m_batchCompatibilityRevision = 1;
    mutable uint64_t m_cachedBatchCompatibilityRevision = 0;
    mutable uint64_t m_cachedBatchCompatibilityHash = 0;
};

using MeshSharedPtr = std::shared_ptr<Mesh>;
} // morrow

#endif //MESH_H
