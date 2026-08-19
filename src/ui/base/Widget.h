#ifndef MORROW_WIDGET_H_
#define MORROW_WIDGET_H_

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "ComponentManager.h"
#include "FrameState.h"
#include "MathUtils.h"
#include "Rect.h"
#include "debug/ObjectRegistry.h"

namespace morrow {
class Interaction;
class MeshRenderer;
struct TouchEvent;
using namespace Math;
class Widget;

class Widget : public std::enable_shared_from_this<Widget> {
public:
    Widget();

    virtual ~Widget();

    void setWidgetName(std::string widgetName);

    virtual void setVisible(bool visible);

    virtual void setDisplayLayer(int32_t displayLayer);

    bool getVisible() const;

    const std::string& getUuid() const;

    const std::string& getWidgetName() const;

    DebugObjectId getDebugObjectId() const;

    /// Snapshot 前同步类型、名称和树关系；仅用于调试，不改变对象所有权。
    void refreshDebugObjectTree(DebugObjectId parentId = 0);

    int32_t getDisplayLayer() const;

    std::string getIdentityInfo();

    /// 每帧由引擎/窗口调用：收集事件目标、更新本节点组件、再递归更新子节点
    virtual void update(FrameStateSharedPtr frameState);

    /// lateUpdate: 在所有 standard update 完成后调用（用于依赖其他组件已更新的逻辑）
    virtual void lateUpdate(FrameStateSharedPtr frameState);

    virtual void onFocusChanged(bool focused);

    /// 将触摸事件派发到本 Widget 的 Interaction 组件（由事件系统调用）
    void dispatchTouchEvent(TouchEvent& event);

    void addChild(std::shared_ptr<Widget> widget);

    bool removeChild(std::shared_ptr<Widget> widget);

    void removeFromStage();

    void requestRender(std::string caller);

    /// ------------------------------------------组件系统接口-------------------------------------------------------

    template <typename T, typename... Args>
    std::shared_ptr<T> addComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        auto component = m_componentManager->addComponent<T>(std::forward<Args>(args)...);
        component->setGameObject(this);
        component->onAttach();
        component->awake();
        return component;
    }

    template <typename T>
    std::shared_ptr<T> getComponent() {
        return m_componentManager->getComponent<T>();
    }

    template <typename T>
    std::shared_ptr<T> getComponent() const {
        return m_componentManager->getComponent<T>();
    }

    template <typename T>
    std::vector<std::shared_ptr<T>> getComponents() {
        return m_componentManager->getComponents<T>();
    }

    template <typename T>
    std::vector<std::shared_ptr<T>> getComponents() const {
        return m_componentManager->getComponents<T>();
    }

    template <typename T>
    void removeComponent() {
        m_componentManager->removeComponent<T>();
    }

    ComponentManager* getComponentManager() const;

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~debug~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    virtual void debug(const std::string& flag);

    virtual void debugTraversal(const std::string& flag);

    virtual void debugTexture();

public:
    std::shared_ptr<Widget> m_parent;
    std::vector<std::shared_ptr<Widget>> m_children;

protected:
    void setWidgetType(std::string widgetType);

    std::string m_widgetName;
    std::string m_widgetType = "MRWidget";
    std::atomic_bool m_visible{true};
    // 显示层级[-10,10]， 动效默认为1
    int32_t m_displayLayer = 0;
    // 组件管理系统
    std::unique_ptr<ComponentManager> m_componentManager;

private:
    DebugObjectHandle m_debugObject{DebugObjectCategory::Widget, "MRWidget"};
    std::string m_uniqueID = Math::generate_uuid();
};
}  // namespace morrow

#endif /* MORROW_WIDGET_H_ */
