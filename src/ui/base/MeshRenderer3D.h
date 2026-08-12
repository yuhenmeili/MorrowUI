//
// Created by lance on 2026/3/31.
//

#ifndef MORROW_GUI_MESHRENDERER3D_H
#define MORROW_GUI_MESHRENDERER3D_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Component.h"
#include "GLTFTypes.h"
#include "Material.h"

namespace morrow {
class RenderDeviceProxyBase;
class VertexArray;

class MeshRenderer3D : public Component {
public:
    MeshRenderer3D();

    ~MeshRenderer3D() override;

    void setFromGLTFMesh(const GLTFMesh& mesh, const std::vector<GLTFMaterial>& materials, const std::vector<TextureSharedPtr>& textures,
                         const std::string& shaderName = "gltf_pbr");

    void update(FrameStateSharedPtr frameState) override;

    uint64_t getRenderRevision() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    uint64_t m_revision = 1;
};
}  // namespace morrow

#endif  // MORROW_GUI_MESHRENDERER3D_H
