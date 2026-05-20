//
// Created by lance on 2026/3/31.
//

#ifndef MORROW_GUI_MESHRENDERER3D_H
#define MORROW_GUI_MESHRENDERER3D_H

#include <vector>
#include <string>
#include <memory>
#include "Component.h"
#include "Material.h"
#include "GLTFTypes.h"

namespace morrow {
class RenderDeviceProxyBase;
class VertexArray;

class MeshRenderer3D : public Component {
public:
    MeshRenderer3D();

    ~MeshRenderer3D() override;

    void setFromGLTFMesh(const GLTFMesh& mesh, const std::vector<GLTFMaterial>& materials, const std::string& shaderName = "gltf_pbr");

    void update(FrameStateSharedPtr frameState) override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace morrow

#endif //MORROW_GUI_MESHRENDERER3D_H
