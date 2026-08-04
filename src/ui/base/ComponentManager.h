#ifndef MORROW_COMPONENT_MANAGER_H
#define MORROW_COMPONENT_MANAGER_H

#include <unordered_map>
#include <vector>
#include <typeindex>
#include <memory>
#include <algorithm>
#include "Component.h"

namespace morrow {

class ComponentManager {
public:
    ComponentManager() = default;
    ~ComponentManager() = default;

    // 添加组件
    template<typename T, typename... Args>
    std::shared_ptr<T> addComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        auto component = std::make_shared<T>(std::forward<Args>(args)...);
        component->setDebugTypeName(debugTypeName<T>());
        auto typeId = std::type_index(typeid(T));

        // 添加到组件列表
        m_components[typeId].push_back(component);
        m_updateOrder.push_back(component);

        return component;
    }

    // 获取组件（按类型）
    template<typename T>
    std::shared_ptr<T> getComponent() const {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        
        auto typeId = std::type_index(typeid(T));
        auto it = m_components.find(typeId);
        if (it != m_components.end() && !it->second.empty()) {
            return std::static_pointer_cast<T>(it->second.front());
        }
        return nullptr;
    }

    // 获取所有指定类型的组件
    template<typename T>
    std::vector<std::shared_ptr<T>> getComponents() const {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        
        std::vector<std::shared_ptr<T>> result;
        auto typeId = std::type_index(typeid(T));
        auto it = m_components.find(typeId);
        if (it != m_components.end()) {
            for (const auto& component : it->second) {
                result.push_back(std::static_pointer_cast<T>(component));
            }
        }
        return result;
    }

    // 移除组件
    template<typename T>
    void removeComponent() {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        auto typeId = std::type_index(typeid(T));
        auto it = m_components.find(typeId);
        if (it != m_components.end()) {
            for (auto& component : it->second) {
                if (component) {
                    component->onDetach();
                    component->onDestroy();
                    auto orderIt = std::find(m_updateOrder.begin(), m_updateOrder.end(), component);
                    if (orderIt != m_updateOrder.end()) {
                        m_updateOrder.erase(orderIt);
                    }
                }
            }
            m_components.erase(typeId);
        }
    }

    // 更新所有启用的组件
    void updateComponents(FrameStateSharedPtr frameState) {
        for (auto& component : m_updateOrder) {
            if (component && component->isEnabled()) {
                component->update(frameState);
            }
        }
    }

    // lateUpdate: 在所有 standard update 完成后调用
    void lateUpdateComponents(FrameStateSharedPtr frameState) {
        for (auto& component : m_updateOrder) {
            if (component && component->isEnabled()) {
                component->lateUpdate(frameState);
            }
        }
    }

    bool requiresContinuousUpdate() const {
        for (const auto& component : m_updateOrder) {
            if (component && component->isEnabled() && component->requiresContinuousUpdate()) {
                return true;
            }
        }
        return false;
    }

private:
    std::unordered_map<std::type_index, std::vector<ComponentSharedPtr>> m_components;
    std::vector<ComponentSharedPtr> m_updateOrder;
};

} // namespace morrow

#endif // MORROW_COMPONENT_MANAGER_H
