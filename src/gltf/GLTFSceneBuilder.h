//
// Created by lance on 2026/3/31.
//

#ifndef MORROW_GUI_GLTFSCENEBUILDER_H
#define MORROW_GUI_GLTFSCENEBUILDER_H

#include <memory>
#include <string>
#include "GLTFTypes.h"

namespace morrow {
class SceneNode;
class RenderDeviceProxyBase;

class GLTFSceneBuilder {
public:
    static std::shared_ptr<SceneNode> build(const std::shared_ptr<GLTFScene>& scene, const std::string& shaderName = "gltf_pbr");

    static bool computeBounds(const std::shared_ptr<GLTFScene>& scene, Vector3& outMin, Vector3& outMax);

private:
    static std::shared_ptr<SceneNode> buildNode(const GLTFScene& scene, int nodeIndex, const std::string& shaderName);
};
} // namespace morrow

#endif //MORROW_GUI_GLTFSCENEBUILDER_H
