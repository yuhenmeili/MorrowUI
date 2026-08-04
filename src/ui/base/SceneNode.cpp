#include "SceneNode.h"

#include <stdexcept>

namespace morrow {

SceneNode::SceneNode()
    : Widget() {
    addComponent<Transform3D>();
    setWidgetType("SceneNode");
}

Transform3DSharedPtr SceneNode::getTransform() {
    return getComponent<Transform3D>();
}

Transform3DSharedPtr SceneNode::getTransform() const {
    return getComponent<Transform3D>();
}

std::shared_ptr<SceneNode> SceneNode::getParentSceneNode() const {
    return std::dynamic_pointer_cast<SceneNode>(m_parent);
}

std::vector<std::shared_ptr<SceneNode>> SceneNode::getSceneChildren() const {
    std::vector<std::shared_ptr<SceneNode>> sceneChildren;
    sceneChildren.reserve(m_children.size());
    for (const auto& child : m_children) {
        if (auto sceneChild = std::dynamic_pointer_cast<SceneNode>(child)) {
            sceneChildren.push_back(sceneChild);
        }
    }
    return sceneChildren;
}

void SceneNode::addSceneChild(const std::shared_ptr<SceneNode>& child) {
    if (!child) return;
    addChild(child);
}

bool SceneNode::removeSceneChild(const std::shared_ptr<SceneNode>& child) {
    if (!child) return false;
    return removeChild(child);
}

bool SceneNode::hasParentSceneNode() const {
    return static_cast<bool>(getParentSceneNode());
}

Transform3DSharedPtr SceneNode::requireTransform() const {
    auto transform = getTransform();
    if (!transform) {
        throw std::runtime_error("SceneNode requires Transform3D");
    }
    return transform;
}

} // namespace morrow

