#ifndef MORROW_GUI_SCENENODE_H
#define MORROW_GUI_SCENENODE_H

#include <vector>

#include "Widget.h"
#include "Transform3D.h"

namespace morrow {

class Transform3D;

class SceneNode : public Widget {
public:
    SceneNode();

    ~SceneNode() override = default;

    Transform3DSharedPtr getTransform();

    Transform3DSharedPtr getTransform() const;

    std::shared_ptr<SceneNode> getParentSceneNode() const;

    std::vector<std::shared_ptr<SceneNode>> getSceneChildren() const;

    void addSceneChild(const std::shared_ptr<SceneNode>& child);

    bool removeSceneChild(const std::shared_ptr<SceneNode>& child);

    bool hasParentSceneNode() const;

private:
    Transform3DSharedPtr requireTransform() const;
};

using SceneNodeSharedPtr = std::shared_ptr<SceneNode>;

} // namespace morrow

#endif // MORROW_GUI_SCENENODE_H
