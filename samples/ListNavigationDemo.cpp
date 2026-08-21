#include "Engine.h"
#include "FontManager.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "elements/MRColor.h"
#include "elements/MRItemList.h"
#include "elements/MRLabel.h"
#include "elements/MRScrollContainer.h"
#include "elements/MRTree.h"

using namespace morrow;

namespace {

std::shared_ptr<MRLabel> createLabel(const std::wstring& text, float x, float y, float width, float height, float fontSize = 20.0f) {
    auto label = std::make_shared<MRLabel>();
    label->setText(text, "default");
    label->setFontSize(fontSize);
    label->setFontColor(0.12f, 0.16f, 0.22f, 1.0f);
    label->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    label->getComponent<Transform>()->setPosition(x, y, 0.0f);
    label->getComponent<Transform>()->setSize(width, height);
    return label;
}

std::shared_ptr<MRColor> createPanel(float x, float y, float width, float height) {
    auto panel = MRColor::create();
    panel->setColor(0.99f, 0.995f, 1.0f, 1.0f);
    panel->setRounding(12.0f);
    panel->getComponent<Transform>()->setPosition(x, y, -0.1f);
    panel->getComponent<Transform>()->setSize(width, height);
    return panel;
}

std::shared_ptr<MRButton> createListItem(
    int index,
    float width,
    float y,
    std::vector<Observable<BaseButton&>::Connection>& connections) {
    auto item = MRButton::create();
    item->setText(L"车辆消息 " + std::to_wstring(index), "default");
    item->setTextFontSize(19.0f);
    item->setTextColor(0.1f, 0.13f, 0.18f, 1.0f);
    item->setTextAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    item->setBackgroundColor(index % 2 == 0 ? Vector4(0.91f, 0.94f, 0.97f, 1.0f) : Vector4(0.96f, 0.97f, 0.99f, 1.0f));
    item->setHoverColor(Vector4(0.78f, 0.87f, 0.94f, 1.0f));
    item->setPressedColor(Vector4(0.67f, 0.8f, 0.9f, 1.0f));
    item->setCornerRadius(6.0f);
    item->getComponent<Transform>()->setPosition(0.0f, y, 0.0f);
    item->getComponent<Transform>()->setSize(width, 48.0f);
    connections.emplace_back(
        item->events().onClicked.connect(
            [index](BaseButton&) {
                LOG_I("Scroll list item clicked: {}", index);
            }));
    return item;
}

}  // namespace

int main() {
    auto engine = std::make_shared<Engine>();
    auto window = engine->getWindow();
    window->setClearColor(0.92f, 0.95f, 0.98f, 1.0f);
    engine->addFonts({FontInfo{
        .name = "default",
        .path = "assets/fonts/MorrowSansCN1.1-Regular.otf",
    }});

    window->addChild(createLabel(L"List Navigation Showcase：ItemList / Tree / ScrollContainer", 60.0f, 30.0f, 1800.0f, 54.0f, 34.0f));
    window->addChild(createLabel(L"列表选择、树形展开折叠和长列表滚动统一放在同一个导航场景中。", 60.0f, 84.0f, 1800.0f, 36.0f, 20.0f));

    constexpr float panelTop = 150.0f;
    constexpr float panelHeight = 760.0f;
    constexpr float itemPanelX = 60.0f;
    constexpr float itemPanelW = 540.0f;
    constexpr float treePanelX = 640.0f;
    constexpr float treePanelW = 540.0f;
    constexpr float scrollPanelX = 1240.0f;
    constexpr float scrollPanelW = 620.0f;

    window->addChild(createPanel(itemPanelX, panelTop, itemPanelW, panelHeight));
    window->addChild(createPanel(treePanelX, panelTop, treePanelW, panelHeight));
    window->addChild(createPanel(scrollPanelX, panelTop, scrollPanelW, panelHeight));

    // ---------------------------------------------------------------------
    // MRItemList
    // ---------------------------------------------------------------------
    window->addChild(createLabel(L"MRItemList", itemPanelX + 24.0f, panelTop + 22.0f, itemPanelW - 48.0f, 42.0f, 25.0f));
    auto listStatus = createLabel(L"当前选择：未选择", itemPanelX + 24.0f, panelTop + 680.0f, itemPanelW - 48.0f, 42.0f, 18.0f);
    window->addChild(listStatus);

    auto itemList = MRItemList::create();
    itemList->getComponent<Transform>()->setPosition(itemPanelX + 24.0f, panelTop + 82.0f, 0.0f);
    itemList->getComponent<Transform>()->setSize(itemPanelW - 48.0f, 560.0f);
    itemList->setItemHeight(48.0f);
    itemList->addItem(L"仪表主题：经典蓝", 101);
    itemList->addItem(L"仪表主题：科技紫", 102);
    itemList->addItem(L"仪表主题：运动红", 103);
    itemList->addItem(L"驾驶模式：舒适", 201);
    itemList->addItem(L"驾驶模式：标准", 202);
    itemList->addItem(L"驾驶模式：运动", 203);
    itemList->addItem(L"驾驶模式：赛道（不可用）", 204, false);
    itemList->addItem(L"氛围灯：冰川蓝", 301);
    itemList->addItem(L"氛围灯：日落橙", 302);
    itemList->addItem(L"氛围灯：森林绿", 303);
    auto itemSelectionConnection =
        itemList->events().onItemSelected.connect(
            [listStatus](MRItemList&, int id, const std::wstring& text) {
                listStatus->setText(
                    L"当前选择：" + text + L"（ID " + std::to_wstring(id) + L"）",
                    "default");
            });
    window->addChild(itemList);

    // ---------------------------------------------------------------------
    // MRTree
    // ---------------------------------------------------------------------
    window->addChild(createLabel(L"MRTree", treePanelX + 24.0f, panelTop + 22.0f, treePanelW - 48.0f, 42.0f, 25.0f));
    auto treeStatus = createLabel(L"当前节点：未选择", treePanelX + 24.0f, panelTop + 680.0f, treePanelW - 48.0f, 42.0f, 18.0f);
    window->addChild(treeStatus);

    auto tree = MRTree::create();
    tree->getComponent<Transform>()->setPosition(treePanelX + 24.0f, panelTop + 82.0f, 0.0f);
    tree->getComponent<Transform>()->setSize(treePanelW - 48.0f, 560.0f);
    tree->addNode(1, L"车辆设置");
    tree->addNode(10, L"驾驶辅助", 1);
    tree->addNode(11, L"车道保持", 10);
    tree->addNode(12, L"自动泊车", 10);
    tree->addNode(20, L"灯光", 1);
    tree->addNode(21, L"自动远光灯", 20);
    tree->addNode(22, L"氛围灯", 20);
    tree->addNode(2, L"多媒体");
    tree->addNode(30, L"音频", 2);
    tree->addNode(31, L"均衡器", 30);
    tree->addNode(32, L"音场定位", 30);
    tree->addNode(40, L"视频", 2);
    tree->addNode(41, L"行车记录仪", 40);
    tree->addNode(42, L"倒车影像", 40);
    tree->addNode(3, L"系统");
    tree->addNode(50, L"显示", 3);
    tree->addNode(51, L"亮度", 50);
    tree->addNode(52, L"深色模式", 50);
    tree->addNode(60, L"开发者选项（不可用）", 3, true, false);
    auto treeSelectionConnection =
        tree->events().onNodeSelected.connect(
            [treeStatus](MRTree&, int id, const std::wstring& text) {
                treeStatus->setText(
                    L"当前节点：" + text + L"（ID " + std::to_wstring(id) + L"）",
                    "default");
            });
    auto treeExpandedConnection =
        tree->events().onNodeExpanded.connect(
            [](MRTree&, int id, bool expanded) {
                LOG_I("Tree node {} expanded = {}", id, expanded);
            });
    window->addChild(tree);

    // ---------------------------------------------------------------------
    // MRScrollContainer
    // ---------------------------------------------------------------------
    window->addChild(createLabel(L"MRScrollContainer", scrollPanelX + 24.0f, panelTop + 22.0f, scrollPanelW - 48.0f, 42.0f, 25.0f));
    constexpr float viewportX = scrollPanelX + 24.0f;
    constexpr float viewportY = panelTop + 82.0f;
    constexpr float viewportW = scrollPanelW - 48.0f;
    constexpr float viewportH = 560.0f;

    auto scroll = MRScrollContainer::create();
    scroll->getComponent<Transform>()->setPosition(viewportX, viewportY, 0.0f);
    scroll->getComponent<Transform>()->setSize(viewportW, viewportH);
    scroll->setScrollStep(56.0f);
    scroll->getVerticalScrollBar()->setTrackColor(Vector4(0.86f, 0.89f, 0.93f, 1.0f));
    scroll->getVerticalScrollBar()->setThumbColor(Vector4(0.24f, 0.52f, 0.7f, 1.0f));
    std::vector<Observable<BaseButton&>::Connection> scrollItemConnections;
    for (int index = 1; index <= 24; ++index) {
        scroll->addScrollChild(
            createListItem(
                index,
                viewportW - 32.0f,
                static_cast<float>(index - 1) * 56.0f,
                scrollItemConnections));
    }
    window->addChild(scroll);
    window->addChild(createLabel(L"滚轮 / 拖动 / 右侧滚动条", scrollPanelX + 24.0f, panelTop + 680.0f, scrollPanelW - 48.0f, 42.0f, 18.0f));

    LOG_I("ListNavigationDemo started");
    engine->render();
    return 0;
}
