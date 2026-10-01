//
// Created by lance on 2026/3/31.
//

#ifndef MORROW_GUI_GLTFSCENEBUILDER_H
#define MORROW_GUI_GLTFSCENEBUILDER_H

#include <memory>
#include <string>

#include "morrow/GLTFTypes.h"
#include "morrow/Texture.h"

namespace morrow {
class SceneNode3D;
class RenderDeviceProxyBase;

class GLTFSceneBuilder {
public:
    static std::shared_ptr<SceneNode3D> build(const std::shared_ptr<GLTFScene>& scene, const std::string& shaderName = "gltf_pbr");

    static bool computeBounds(const std::shared_ptr<GLTFScene>& scene, Vector3& outMin, Vector3& outMax);

private:
    static std::shared_ptr<SceneNode3D> buildNode(const GLTFScene& scene, const std::vector<TextureSharedPtr>& textures, int nodeIndex, const std::string& shaderName);
};
}  // namespace morrow

#endif  // MORROW_GUI_GLTFSCENEBUILDER_H
