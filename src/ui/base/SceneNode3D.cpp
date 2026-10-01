#include "morrow/base/SceneNode3D.h"

#include <stdexcept>

namespace morrow {

SceneNode3D::SceneNode3D()
    : Widget() {
    addComponent<Transform3D>();
    setWidgetType("SceneNode3D");
}

Transform3DSharedPtr SceneNode3D::getTransform() {
    return getComponent<Transform3D>();
}

Transform3DSharedPtr SceneNode3D::getTransform() const {
    return getComponent<Transform3D>();
}

std::shared_ptr<SceneNode3D> SceneNode3D::getParentSceneNode() const {
    return std::dynamic_pointer_cast<SceneNode3D>(m_parent);
}

std::vector<std::shared_ptr<SceneNode3D>> SceneNode3D::getSceneChildren() const {
    std::vector<std::shared_ptr<SceneNode3D>> sceneChildren;
    sceneChildren.reserve(m_children.size());
    for (const auto& child : m_children) {
        if (auto sceneChild = std::dynamic_pointer_cast<SceneNode3D>(child)) {
            sceneChildren.push_back(sceneChild);
        }
    }
    return sceneChildren;
}

void SceneNode3D::addSceneChild(const std::shared_ptr<SceneNode3D>& child) {
    if (!child) return;
    addChild(child);
}

bool SceneNode3D::removeSceneChild(const std::shared_ptr<SceneNode3D>& child) {
    if (!child) return false;
    return removeChild(child);
}

bool SceneNode3D::hasParentSceneNode() const {
    return static_cast<bool>(getParentSceneNode());
}

Transform3DSharedPtr SceneNode3D::requireTransform() const {
    auto transform = getTransform();
    if (!transform) {
        throw std::runtime_error("SceneNode3D requires Transform3D");
    }
    return transform;
}

} // namespace morrow

