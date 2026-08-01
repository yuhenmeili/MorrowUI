#include "Component.h"
#include "Widget.h"
#include "Transform.h"
#include "Transform3D.h"
#include "Component.inl"

namespace morrow {
void Component::onAttach() {
}

void Component::awake() {
}

void Component::start() {
}

void Component::update(FrameStateSharedPtr frameState) {
}

void Component::lateUpdate(FrameStateSharedPtr frameState) {
}

void Component::onEnable() {
}

void Component::onDisable() {
}

void Component::onDetach() {
}

void Component::onDestroy() {
}

void Component::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    if (enabled) {
        onEnable();
    } else {
        onDisable();
    }
}

bool Component::isEnabled() const {
    return m_enabled;
}

Widget* Component::getGameObject() const {
    return m_gameObject;
}

void Component::setGameObject(Widget* gameObject) {
    m_gameObject = gameObject;
}

// Explicit instantiations for commonly used transform component lookups.
template std::shared_ptr<Transform> Component::getComponent<Transform>();

template std::shared_ptr<Transform> Component::getComponentInParent<Transform>();

template std::shared_ptr<Transform3D> Component::getComponent<Transform3D>();

template std::shared_ptr<Transform3D> Component::getComponentInParent<Transform3D>();
} // namespace morrow