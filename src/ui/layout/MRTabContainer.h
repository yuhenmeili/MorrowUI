#ifndef MORROW_MR_TAB_CONTAINER_H
#define MORROW_MR_TAB_CONTAINER_H

#include <memory>
#include <string>
#include <vector>

#include "base/UIWidget.h"
#include "core/Observable.h"

namespace morrow {

class BaseButton;
class MRButton;

class MRTabContainer : public UIWidget {
public:
    struct Tab {
        std::string id;
        std::wstring title;
        std::shared_ptr<UIWidget> content;
        std::shared_ptr<MRButton> button;
    };

    struct Events {
        Observable<MRTabContainer&, const std::string&> onCurrentTabChanged;
        Observable<MRTabContainer&> onTabOrderChanged;
    };

    static std::shared_ptr<MRTabContainer> create();

    bool addTab(const std::string& id, const std::wstring& title, const std::shared_ptr<UIWidget>& content);

    bool removeTab(const std::string& id);

    bool selectTab(const std::string& id);

    bool moveTab(size_t from, size_t to);

    const std::string& currentTabId() const;

    const std::vector<Tab>& tabs() const;

    Events& events();

    void update(FrameStateSharedPtr frameState) override;

private:
    MRTabContainer();

    void layoutTabs();
    void updateTabStyles();
    int findTabIndex(const std::string& id) const;

    std::vector<Tab> m_tabs;
    std::vector<Observable<BaseButton&>::Connection> m_tabConnections;
    Events m_events;
    std::string m_currentTabId;
    float m_tabBarHeight = 30.0f;
    float m_tabSpacing = 2.0f;
};

using MRTabContainerSharedPtr = std::shared_ptr<MRTabContainer>;

}  // namespace morrow

#endif  // MORROW_MR_TAB_CONTAINER_H
