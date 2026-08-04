#include "Widget.h"
#include "utils/Log.h"
#include "Rect.h"
#include "GlobalObject.h"
#include "Interaction.h"
#include "TouchEvent.h"
#include "core/OrthographicCamera.h"

namespace morrow
{
bool sortChildren(const std::shared_ptr<Widget>& a, const std::shared_ptr<Widget>& b)
{
    return (a->getDisplayLayer() < b->getDisplayLayer());
}

Widget::Widget()
    : m_componentManager(std::make_unique<ComponentManager>()) {
}

Widget::~Widget() = default;

void Widget::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        requestRender("setVisible");
        debug("setVisible");
    }
}

bool Widget::getVisible() const
{
    if (!m_visible) return false;
    if (m_parent) return m_parent->getVisible();
    return true;
}

void Widget::update(FrameStateSharedPtr frameState)
{
    m_componentManager->updateComponents(frameState);
    for (auto& child : m_children) {
        if (child->getVisible()) {
            child->update(frameState);
        }
    }
}

void Widget::lateUpdate(FrameStateSharedPtr frameState)
{
    m_componentManager->lateUpdateComponents(frameState);
    for (auto& child : m_children) {
        if (child->getVisible()) {
            child->lateUpdate(frameState);
        }
    }
}

const std::string& Widget::getUuid() const
{
    return m_uniqueID;
}

void Widget::setWidgetName(std::string widgetName)
{
    m_widgetName = std::move(widgetName);
    m_debugObject.setName(m_widgetName);
}

const std::string& Widget::getWidgetName() const
{
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
        if (child) child->refreshDebugObjectTree(selfId);
    }
}

void Widget::addChild(std::shared_ptr<Widget> widget)
{
    widget->removeFromStage();
    widget->m_parent = shared_from_this();
    widget->m_debugObject.setParentId(getDebugObjectId());
    m_children.emplace_back(widget);
    std::stable_sort(m_children.begin(), m_children.end(), sortChildren);
    requestRender("addChild");
}

bool Widget::removeChild(std::shared_ptr<Widget> widget)
{
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

int32_t Widget::getDisplayLayer() const
{
    return m_displayLayer;
}

void Widget::setDisplayLayer(int32_t displayLayer)
{
    if (m_displayLayer != displayLayer) {
        m_displayLayer = Math::clamp(displayLayer, -10, 10);
        if (m_parent) {
            std::stable_sort(m_parent->m_children.begin(), m_parent->m_children.end(), sortChildren);
        }
        LOG_D("{}, {}", m_widgetName.c_str(), m_displayLayer);
        requestRender("setDisplayLayer");
    }
}

std::string Widget::getIdentityInfo()
{
    return m_uniqueID + "_" + m_widgetType + "_" + m_widgetName;
}


void Widget::dispatchTouchEvent(TouchEvent& event)
{
    auto interaction = getComponent<Interaction>();
    if (interaction) interaction->handleTouchEvent(event);
}

void Widget::removeFromStage()
{
    if (m_parent) {
        m_parent->removeChild(shared_from_this());
    }
}

void Widget::requestRender(std::string caller)
{
    REQUESTRENDER;
}

ComponentManager* Widget::getComponentManager() const { return m_componentManager.get(); }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~debug~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Widget::debug(const std::string& flag)
{
    LOG_I("{}, {}, visible: {}",
          getIdentityInfo().c_str(),
          flag.c_str(),
          getVisible());
}

void Widget::debugTraversal(const std::string& flag)
{
    debug(flag);
    if (!m_children.empty()) {
        for (const auto& child : m_children) {
            if (child->getVisible()) {
                child->debugTraversal(flag);
            }
        }
    }
}

void Widget::debugTexture()
{
    if (!m_children.empty()) {
        for (const auto& child : m_children) {
            if (child->getVisible()) {
                child->debugTexture();
            }
        }
    }
}

}
