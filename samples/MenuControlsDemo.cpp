#include "Engine.h"
#include "FontManager.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "elements/MRLabel.h"
#include "elements/MRMenuButton.h"
#include "elements/MROptionButton.h"
#include "elements/MRPopupMenu.h"

using namespace morrow;

namespace {

template <typename T>
void configureButton(const std::shared_ptr<T>& button, const std::wstring& text, float x, float y, float width) {
    button->setText(text, "default");
    button->setTextFontSize(20.0f);
    button->setTextColor(0.1f, 0.12f, 0.16f, 1.0f);
    button->setBackgroundColor(0.9f, 0.92f, 0.95f, 1.0f);
    button->setHoverColor(Vector4(0.82f, 0.88f, 0.94f, 1.0f));
    button->setPressedColor(Vector4(0.72f, 0.82f, 0.92f, 1.0f));
    button->setCornerRadius(6.0f);
    button->template getComponent<Transform>()->setPosition(x, y, 0.0f);
    button->template getComponent<Transform>()->setSize(width, 48.0f);
}

std::shared_ptr<MRLabel> createTitle(const std::wstring& text, float x, float y, float width) {
    auto label = std::make_shared<MRLabel>();
    label->setText(text, "default");
    label->setFontSize(18.0f);
    label->setFontColor(0.3f, 0.34f, 0.4f, 1.0f);
    label->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    label->getComponent<Transform>()->setPosition(x, y, 0.0f);
    label->getComponent<Transform>()->setSize(width, 28.0f);
    return label;
}

}  // namespace

int main() {
    EngineOptions options;
    // options.windowInfo.width = 760;
    // options.windowInfo.height = 420;
    auto engine = std::make_shared<Engine>(options);
    auto window = engine->getWindow();
    window->setClearColor(0.97f, 0.98f, 1.0f, 1.0f);

    FontInfo fontInfo = {.name = "default", .path = "assets/fonts/MorrowSansCN1.1-Regular.otf"};
    engine->addFonts({fontInfo});

    constexpr float leftX = 34.0f;
    constexpr float rightX = 394.0f;
    constexpr float controlWidth = 300.0f;

    window->addChild(createTitle(L"OptionButton 下拉选择", leftX, 28.0f, controlWidth));
    auto optionButton = MROptionButton::create();
    configureButton(optionButton, L"请选择驾驶员", leftX, 66.0f, controlWidth);
    optionButton->addOption(L"驾驶员 A", 101);
    optionButton->addOption(L"驾驶员 B", 102);
    optionButton->addOption(L"访客模式", 103);
    optionButton->getPopupMenu()->setMenuWidth(controlWidth);
    optionButton->setOnSelectedCallback([](int id, const std::wstring&) { LOG_I("OptionButton selected id = {}", id); });
    window->addChild(optionButton);

    window->addChild(createTitle(L"MenuButton 操作菜单", rightX, 28.0f, controlWidth));
    auto menuButton = MRMenuButton::create();
    configureButton(menuButton, L"车辆操作", rightX, 66.0f, controlWidth);
    menuButton->addMenuItem(L"保存当前设置", 201);
    menuButton->addMenuItem(L"恢复默认设置", 202);
    menuButton->addMenuItem(L"导出配置", 203);
    menuButton->getPopupMenu()->setMenuWidth(controlWidth);
    menuButton->setOnMenuItemSelectedCallback([](int id, const std::wstring&) { LOG_I("MenuButton selected id = {}", id); });
    window->addChild(menuButton);

    window->addChild(createTitle(L"PopupMenu 独立弹出", leftX, 254.0f, controlWidth));
    auto popupMenu = MRPopupMenu::create();
    popupMenu->setMenuWidth(controlWidth);
    popupMenu->addItem(L"打开诊断页面", 301);
    popupMenu->addItem(L"查看系统信息", 302);
    popupMenu->addItem(L"维护模式（禁用）", 303, false);
    popupMenu->setOnItemSelectedCallback([](int id, const std::wstring&) { LOG_I("PopupMenu selected id = {}", id); });
    popupMenu->attachTo(window);

    auto popupButton = MRButton::create();
    configureButton(popupButton, L"打开独立菜单", leftX, 292.0f, controlWidth);
    popupButton->setOnClickCallback([popupMenu, popupButton]() {
        if (popupMenu->isOpen()) {
            popupMenu->hide();
        } else {
            popupMenu->popupBelow(popupButton->getScreenSpaceAABB());
        }
    });
    window->addChild(popupButton);

    LOG_I("Menu controls demo started");
    engine->render();
    return 0;
}
