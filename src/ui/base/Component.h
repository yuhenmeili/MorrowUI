#ifndef MORROW_COMPONENT_H
#define MORROW_COMPONENT_H

#include <memory>
#include <string>
#include "FrameState.h"

namespace morrow {
class Widget;
class Transform;
class Transform3D;

class Component : public std::enable_shared_from_this<Component> {
public:
    Component() = default;

    virtual ~Component() = default;

    // 组件生命周期方法
    virtual void awake();

    virtual void start();

    virtual void update(FrameStateSharedPtr frameState);

    // 当组件被添加到激活的 GameObject 时调用
    virtual void onEnable();

    // 当组件被移除 或 所在 GameObject 失活时调用
    virtual void onDisable();

    virtual void onDestroy();

    // 组件启用/禁用
    void setEnabled(bool enabled);

    bool isEnabled() const;

    // 获取所属的 GameObject
    Widget* getGameObject() const;

    void setGameObject(Widget* gameObject);

    template <typename T>
    std::shared_ptr<T> getComponent();

    template <typename T>
    std::shared_ptr<T> getComponentInParent();

protected:
    bool m_enabled = true;

private:
    Widget* m_gameObject = nullptr;
};

using ComponentSharedPtr = std::shared_ptr<Component>;
} // namespace morrow

#endif // MORROW_COMPONENT_H
