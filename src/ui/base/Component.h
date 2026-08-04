#ifndef MORROW_COMPONENT_H
#define MORROW_COMPONENT_H

#include <memory>
#include <string>
#include "FrameState.h"
#include "debug/ObjectRegistry.h"

namespace morrow {
class Widget;
class Transform;
class Transform3D;

class Component : public std::enable_shared_from_this<Component> {
public:
    Component() = default;

    virtual ~Component();

    /// ────────── 生命周期（调用于 ComponentManager / Widget）──────────
    /// onAttach:   组件被 addComponent 添加到 Widget 后
    /// awake:      组件首次激活时（Widget 首次 update 前）
    /// start:      组件首次激活时（awake 之后，首帧 update 之前）
    /// update:     每帧调用（仅 enabled 组件）
    /// lateUpdate: 每帧在所有 update 完成后调用（仅 enabled 组件）
    /// onEnable:   setEnabled(true) 时
    /// onDisable:  setEnabled(false) 时
    /// onDetach:   组件被 removeComponent 从 Widget 移除前
    /// onDestroy:  组件销毁时
    virtual void onAttach();

    virtual void awake();

    virtual void start();

    virtual void update(FrameStateSharedPtr frameState);

    virtual void lateUpdate(FrameStateSharedPtr frameState);

    virtual void onEnable();

    virtual void onDisable();

    virtual void onDetach();

    virtual void onDestroy();

    /// Whether the owning scene must keep updating while this component is enabled.
    virtual bool requiresContinuousUpdate() const;

    // 组件启用/禁用
    void setEnabled(bool enabled);

    bool isEnabled() const;

    // 获取所属的 GameObject
    Widget* getGameObject() const;

    void setGameObject(Widget* gameObject);

    DebugObjectId getDebugObjectId() const;

    void setDebugTypeName(const std::string& typeName);

    template <typename T>
    std::shared_ptr<T> getComponent();

    template <typename T>
    std::shared_ptr<T> getComponentInParent();

protected:
    bool m_enabled = true;

private:
    Widget* m_gameObject = nullptr;
    DebugObjectHandle m_debugObject{DebugObjectCategory::Component, "Component"};
};

using ComponentSharedPtr = std::shared_ptr<Component>;
} // namespace morrow

#endif // MORROW_COMPONENT_H
