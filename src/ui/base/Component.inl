#ifndef MORROW_COMPONENT_INL
#define MORROW_COMPONENT_INL

#include "Widget.h" // 确保Widget的完整定义可见

namespace morrow {

template <typename T>
std::shared_ptr<T> Component::getComponent() {
    if (m_gameObject) {
        return m_gameObject->getComponent<T>();
    }
    return nullptr;
}

template <typename T>
std::shared_ptr<T> Component::getComponentInParent() {
    if (m_gameObject && m_gameObject->m_parent) {
        return m_gameObject->m_parent->getComponent<T>();
    }
    return nullptr;
}

} // namespace morrow

#endif // MORROW_COMPONENT_INL