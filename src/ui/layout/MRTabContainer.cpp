#include "MRTabContainer.h"

#include <algorithm>

#include "base/BaseButton.h"
#include "base/Transform.h"
#include "elements/MRButton.h"

namespace morrow {

std::shared_ptr<MRTabContainer> MRTabContainer::create() {
    return std::shared_ptr<MRTabContainer>(new MRTabContainer());
}

MRTabContainer::MRTabContainer() : UIWidget(false) {
    setWidgetType("MRTabContainer");
    setClipChildren(true);
    getTransform()->addSizeChangeListener([this]() { layoutTabs(); });
}

bool MRTabContainer::addTab(const std::string& id, const std::wstring& title, const std::shared_ptr<UIWidget>& content) {
    if (id.empty() || !content || findTabIndex(id) >= 0)
        return false;

    Tab tab;
    tab.id = id;
    tab.title = title;
    tab.content = content;
    tab.button = MRButton::create();
    tab.button->setText(title, "default");
    tab.button->setTextFontSize(14.0f);
    tab.button->setAutoWrap(false);
    tab.button->setTextAlign(HorizontalAlignment::CENTER, VerticalAlignment::CENTER);
    tab.button->setCornerRadius(2.0f);
    tab.button->setDisplayLayer(5);
    const std::string tabId = id;
    m_tabConnections.emplace_back(tab.button->events().onClicked.connect([this, tabId](BaseButton&) { selectTab(tabId); }));

    addChild(tab.content);
    addChild(tab.button);
    tab.content->setVisible(false);
    m_tabs.push_back(std::move(tab));
    if (m_currentTabId.empty())
        m_currentTabId = id;
    layoutTabs();
    return true;
}

bool MRTabContainer::removeTab(const std::string& id) {
    const int index = findTabIndex(id);
    if (index < 0)
        return false;

    const size_t removed = static_cast<size_t>(index);
    removeChild(m_tabs[removed].button);
    removeChild(m_tabs[removed].content);
    m_tabs.erase(m_tabs.begin() + index);
    m_tabConnections.erase(m_tabConnections.begin() + index);

    if (m_tabs.empty()) {
        m_currentTabId.clear();
    } else if (m_currentTabId == id) {
        const size_t nextIndex = std::min(removed, m_tabs.size() - 1);
        m_currentTabId = m_tabs[nextIndex].id;
    }
    layoutTabs();
    m_events.onCurrentTabChanged.notify(*this, m_currentTabId);
    return true;
}

bool MRTabContainer::selectTab(const std::string& id) {
    if (findTabIndex(id) < 0 || m_currentTabId == id)
        return findTabIndex(id) >= 0;
    m_currentTabId = id;
    layoutTabs();
    m_events.onCurrentTabChanged.notify(*this, m_currentTabId);
    return true;
}

bool MRTabContainer::moveTab(size_t from, size_t to) {
    if (from >= m_tabs.size() || to >= m_tabs.size() || from == to)
        return false;
    auto tab = std::move(m_tabs[from]);
    auto connection = std::move(m_tabConnections[from]);
    m_tabs.erase(m_tabs.begin() + static_cast<ptrdiff_t>(from));
    m_tabConnections.erase(m_tabConnections.begin() + static_cast<ptrdiff_t>(from));
    m_tabs.insert(m_tabs.begin() + static_cast<ptrdiff_t>(to), std::move(tab));
    m_tabConnections.insert(m_tabConnections.begin() + static_cast<ptrdiff_t>(to), std::move(connection));
    layoutTabs();
    m_events.onTabOrderChanged.notify(*this);
    return true;
}

const std::string& MRTabContainer::currentTabId() const {
    return m_currentTabId;
}

const std::vector<MRTabContainer::Tab>& MRTabContainer::tabs() const {
    return m_tabs;
}

MRTabContainer::Events& MRTabContainer::events() {
    return m_events;
}

void MRTabContainer::update(FrameStateSharedPtr frameState) {
    layoutTabs();
    UIWidget::update(frameState);
}

void MRTabContainer::layoutTabs() {
    const Vector3 size = getTransform()->getSize();
    float x = 0.0f;
    for (auto& tab : m_tabs) {
        const float width = std::max(72.0f, tab.button->getPreferredSize().x + 18.0f);
        tab.button->getTransform()->setPosition(x, 0.0f, 0.0f);
        tab.button->getTransform()->setSize(std::min(width, std::max(1.0f, size.x - x)), m_tabBarHeight);
        tab.content->getTransform()->setPosition(0.0f, m_tabBarHeight, 0.0f);
        tab.content->getTransform()->setSize(size.x, std::max(1.0f, size.y - m_tabBarHeight));
        tab.content->setVisible(tab.id == m_currentTabId);
        x += width + m_tabSpacing;
    }
    updateTabStyles();
}

void MRTabContainer::updateTabStyles() {
    for (auto& tab : m_tabs) {
        const bool active = tab.id == m_currentTabId;
        tab.button->setTextColor(active ? Vector4(1.0f, 1.0f, 1.0f, 1.0f) : Vector4(0.66f, 0.72f, 0.80f, 1.0f));
        tab.button->setBackgroundColor(active ? Vector4(0.18f, 0.34f, 0.60f, 1.0f) : Vector4(0.11f, 0.14f, 0.19f, 1.0f));
        tab.button->setHoverColor(active ? Vector4(0.23f, 0.43f, 0.72f, 1.0f) : Vector4(0.18f, 0.23f, 0.31f, 1.0f));
        tab.button->setPressedColor(Vector4(0.12f, 0.25f, 0.46f, 1.0f));
    }
}

int MRTabContainer::findTabIndex(const std::string& id) const {
    for (size_t index = 0; index < m_tabs.size(); ++index) {
        if (m_tabs[index].id == id)
            return static_cast<int>(index);
    }
    return -1;
}

}  // namespace morrow
