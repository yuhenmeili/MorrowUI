//
// SceneNode3D — 3D 场景图节点（公共）：持有 Transform3D 的场景树节点，
// GLTF 加载与 3D 子场景构建的基本单元；场景根节点也是本类型的一个实例。
// 2D UI 树根是 Root2D（Engine::getRootWidget()），两者分属不同树。
//

#ifndef MORROW_BASE_SCENENODE3D_H
#define MORROW_BASE_SCENENODE3D_H

#include <vector>

#include "morrow/base/Widget.h"
#include "morrow/base/Transform3D.h"

namespace morrow {

class Transform3D;

class SceneNode3D : public Widget {
public:
    SceneNode3D();

    ~SceneNode3D() override = default;

    Transform3DSharedPtr getTransform();

    Transform3DSharedPtr getTransform() const;

    std::shared_ptr<SceneNode3D> getParentSceneNode() const;

    std::vector<std::shared_ptr<SceneNode3D>> getSceneChildren() const;

    void addSceneChild(const std::shared_ptr<SceneNode3D>& child);

    bool removeSceneChild(const std::shared_ptr<SceneNode3D>& child);

    bool hasParentSceneNode() const;

private:
    Transform3DSharedPtr requireTransform() const;
};

using SceneNode3DSharedPtr = std::shared_ptr<SceneNode3D>;

} // namespace morrow

#endif // MORROW_BASE_SCENENODE3D_H
