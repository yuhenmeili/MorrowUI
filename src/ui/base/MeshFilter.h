#ifndef MESH_FILTER_H
#define MESH_FILTER_H

#include "Component.h"
#include "Mesh.h"
#include "Vector3.h"

namespace morrow {

class MeshFilter : public Component {
public:
    MeshFilter();

    ~MeshFilter() override = default;

    void awake() override;

    void setUVData(const Vector2& uv0, const Vector2& uv1);

    void setUVData(float x0, float y0, float x1, float y1);

    void setColor(const Vector4& color);

    void setColor(float r, float g, float b, float a);
    
    // 设置网格
    void setMesh(const MeshSharedPtr& mesh);
    
    // 获取网格
    MeshSharedPtr getMesh() const;

    uint64_t getGeometryRevision() const;
    
private:
    MeshSharedPtr m_mesh;
    Vector3 m_lastSize;
    Vector2 m_uv0 = {0.0f, 0.0f};
    Vector2 m_uv1 = {1.0f, 1.0f};
    Vector4 m_color = {1.0f, 1.0f, 1.0f, 1.0f};
};

using MeshFilterSharedPtr = std::shared_ptr<MeshFilter>;

} // morrow

#endif // MESH_FILTER_H
