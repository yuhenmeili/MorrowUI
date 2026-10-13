#include "morrow/base/Widget.h"

#include "GlobalObject.h"
#include "morrow/base/Interaction.h"
#include "Rect.h"
#include "morrow/base/TouchEvent.h"
#include "morrow/OrthographicCamera.h"
#include "morrow/utils/Log.h"

namespace morrow {
bool sortChildren(const std::shared_ptr<Widget>& a, const std::shared_ptr<Widget>& b) {
    return (a->getDisplayLayer() < b->getDisplayLayer());
}

Widget::Widget() : m_componentManager(std::make_unique<ComponentManager>()) {
}

Widget::~Widget() = default;

void Widget::setVisible(bool visible) {
    if (m_visible != visible) {
        m_visible = visible;
        requestRender("setVisible");
        debug("setVisible");
    }
}

bool Widget::getVisible() const {
    if (!m_visible)
        return false;
    if (m_parent)
        return m_parent->getVisible();
    return true;
}

void Widget::update(FrameStateSharedPtr frameState) {
    m_componentManager->updateComponents(frameState);
    for (auto& child : m_children) {
        if (child->getVisible()) {
            child->update(frameState);
        }
    }
}

void Widget::lateUpdate(FrameStateSharedPtr frameState) {
    m_componentManager->lateUpdateComponents(frameState);
    for (auto& child : m_children) {
        if (child->getVisible()) {
            child->lateUpdate(frameState);
        }
    }
}

const std::string& Widget::getUuid() const {
    return m_uniqueID;
}

void Widget::setWidgetName(std::string widgetName) {
    m_widgetName = std::move(widgetName);
    m_debugObject.setName(m_widgetName);
}

void Widget::onFocusChanged(bool /*focused*/) {
}

const std::string& Widget::getWidgetName() const {
    return m_widgetName;
}

DebugObjectId Widget::getDebugObjectId() const {
    return m_debugObject.id();
}

void Widget::setWidgetType(std::string widgetType) {
    m_widgetType = std::move(widgetType);
    m_debugObject.setTypeName(m_widgetType);
}

void Widget::refreshDebugObjectTree(DebugObjectId parentId) {
    m_debugObject.setTypeName(m_widgetType);
    m_debugObject.setName(m_widgetName);
    m_debugObject.setParentId(parentId);
    const auto selfId = getDebugObjectId();
    for (const auto& child : m_children) {
        if (child)
            child->refreshDebugObjectTree(selfId);
    }
}

void Widget::addChild(std::shared_ptr<Widget> widget) {
    widget->removeFromStage();
    widget->m_parent = shared_from_this();
    widget->m_debugObject.setParentId(getDebugObjectId());
    m_children.emplace_back(widget);
    std::stable_sort(m_children.begin(), m_children.end(), sortChildren);
    // 层级跟随（挂载）：非零层父节点收编仍处于默认层的子节点，覆盖
    // "先设置父层、后挂载子控件"的顺序；显式分层的子节点不受影响
    if (m_displayLayer != 0 && widget->m_displayLayer == 0) {
        widget->setDisplayLayer(m_displayLayer);
    }
    requestRender("addChild");
}

bool Widget::removeChild(std::shared_ptr<Widget> widget) {
    if (!m_children.empty()) {
        auto iter = std::find(m_children.begin(), m_children.end(), widget);
        if (iter != m_children.end()) {
            widget->m_parent = nullptr;
            widget->m_debugObject.setParentId(0);
            m_children.erase(iter);
            requestRender("removeChild");
            return true;
        }
    }
    return false;
}

int32_t Widget::getDisplayLayer() const {
    return m_displayLayer;
}

void Widget::setDisplayLayer(int32_t displayLayer) {
    const int32_t previousLayer = m_displayLayer;
    if (previousLayer == displayLayer) {
        return;
    }
    m_displayLayer = Math::clamp(displayLayer, -10, 10);
    if (m_parent) {
        std::stable_sort(m_parent->m_children.begin(), m_parent->m_children.end(), sortChildren);
    }
    // 层级跟随（传播）：仍处于父节点原层的子节点（未显式单独分层的默认
    // 子节点）跟随父层变化；显式放在其它层的子节点（如滚动条相对 +2）
    // 保持不变。传播内部不做任何排序，结束后统一排一次——否则逐子节点
    // 的中间态排序会打乱同层兄弟的绘制顺序（如滚动条 track 压住 thumb）。
    propagateDisplayLayer(previousLayer, m_displayLayer);
    std::stable_sort(m_children.begin(), m_children.end(), sortChildren);
    LOG_D("{}, {}", m_widgetName.c_str(), m_displayLayer);
    requestRender("setDisplayLayer");
}

void Widget::propagateDisplayLayer(int32_t previousLayer, int32_t newLayer) {
    // 递归期间没有任何路径会增删/重排 m_children，可直接遍历
    for (const auto& child : m_children) {
        if (child->m_displayLayer == previousLayer) {
            child->m_displayLayer = newLayer; // 父层已 clamp，直接赋值
            child->propagateDisplayLayer(previousLayer, newLayer);
        }
    }
}

std::string Widget::getIdentityInfo() {
    return m_uniqueID + "_" + m_widgetType + "_" + m_widgetName;
}

void Widget::dispatchTouchEvent(TouchEvent& event) {
    auto interaction = getComponent<Interaction>();
    if (interaction)
        interaction->handleTouchEvent(event);
}

void Widget::removeFromStage() {
    if (m_parent) {
        m_parent->removeChild(shared_from_this());
    }
}

void Widget::requestRender(std::string caller) {
    REQUESTRENDER;
}

ComponentManager* Widget::getComponentManager() const {
    return m_componentManager.get();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~debug~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Widget::debug(const std::string& flag) {
    LOG_I("{}, {}, visible: {}", getIdentityInfo().c_str(), flag.c_str(), getVisible());
}

void Widget::debugTraversal(const std::string& flag) {
    debug(flag);
    if (!m_children.empty()) {
        for (const auto& child : m_children) {
            if (child->getVisible()) {
                child->debugTraversal(flag);
            }
        }
    }
}

void Widget::debugTexture() {
    if (!m_children.empty()) {
        for (const auto& child : m_children) {
            if (child->getVisible()) {
                child->debugTexture();
            }
        }
    }
}

}  // namespace morrow
