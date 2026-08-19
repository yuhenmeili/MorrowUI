#include "Engine.h"
#include "FontManager.h"
#include "base/Transform.h"
#include "elements/MRColor.h"
#include "elements/MRItemList.h"
#include "elements/MRLabel.h"
#include "elements/MRTree.h"

using namespace morrow;

namespace {

std::shared_ptr<MRLabel> createLabel(
    const std::wstring& text, float x, float y, float width, float height, float fontSize) {
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
    panel->setColor(0.96f, 0.98f, 1.0f, 1.0f);
    panel->setRounding(12.0f);
    panel->getComponent<Transform>()->setPosition(x, y, -0.1f);
    panel->getComponent<Transform>()->setSize(width, height);
    return panel;
}

}  // namespace

int main() {
    auto engine = std::make_shared<Engine>();
    auto window = engine->getWindow();
    window->setClearColor(0.88f, 0.92f, 0.96f, 1.0f);
    engine->addFonts({FontInfo{.name = "default", .path = "assets/fonts/MorrowSansCN1.1-Regular.otf"}});

    window->addChild(createLabel(L"ItemList / Tree 列表与树形菜单", 80.0f, 42.0f, 1000.0f, 54.0f, 34.0f));
    window->addChild(createLabel(L"鼠标滚轮可滚动；点击树节点可选中并展开或折叠。", 80.0f, 96.0f, 1000.0f, 40.0f, 20.0f));

    constexpr float panelTop = 180.0f;
    constexpr float panelHeight = 720.0f;
    window->addChild(createPanel(80.0f, panelTop, 780.0f, panelHeight));
    window->addChild(createPanel(920.0f, panelTop, 920.0f, panelHeight));
    window->addChild(createLabel(L"MRItemList", 110.0f, panelTop + 22.0f, 400.0f, 46.0f, 26.0f));
    window->addChild(createLabel(L"MRTree", 950.0f, panelTop + 22.0f, 400.0f, 46.0f, 26.0f));

    auto listStatus = createLabel(L"当前选择：未选择", 110.0f, panelTop + 624.0f, 700.0f, 46.0f, 21.0f);
    window->addChild(listStatus);

    auto itemList = MRItemList::create();
    itemList->getComponent<Transform>()->setPosition(110.0f, panelTop + 84.0f, 0.0f);
    itemList->getComponent<Transform>()->setSize(700.0f, 510.0f);
    itemList->setItemHeight(52.0f);
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
    itemList->setOnItemSelectedCallback([listStatus](int id, const std::wstring& text) {
        listStatus->setText(L"当前选择：" + text + L"（ID " + std::to_wstring(id) + L"）", "default");
    });
    window->addChild(itemList);

    auto treeStatus = createLabel(L"当前节点：未选择", 950.0f, panelTop + 624.0f, 820.0f, 46.0f, 21.0f);
    window->addChild(treeStatus);

    auto tree = MRTree::create();
    tree->getComponent<Transform>()->setPosition(950.0f, panelTop + 84.0f, 0.0f);
    tree->getComponent<Transform>()->setSize(840.0f, 510.0f);
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
    tree->setOnNodeSelectedCallback([treeStatus](int id, const std::wstring& text) {
        treeStatus->setText(L"当前节点：" + text + L"（ID " + std::to_wstring(id) + L"）", "default");
    });
    tree->setOnNodeExpandedCallback([](int id, bool expanded) {
        LOG_I("Tree node {} expanded = {}", id, expanded);
    });
    window->addChild(tree);

    LOG_I("ItemListTreeDemo started");
    engine->render();
    return 0;
}
