#include <iostream>
#include <memory>
#include <string>

#include "ui/base/Transform.h"
#include "ui/base/UIWidget.h"
#include "ui/elements/MRMenuButton.h"
#include "ui/elements/MROptionButton.h"
#include "ui/elements/MRPopupMenu.h"

using namespace morrow;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

void click(const std::shared_ptr<BaseButton>& button) {
    button->onMouseDown();
    button->onMouseUp();
}

void testOptionButton() {
    auto root = std::make_shared<UIWidget>(false);
    root->getComponent<Transform>()->setSize(640.0f, 360.0f);

    auto option = MROptionButton::create();
    option->getComponent<Transform>()->setPosition(20.0f, 20.0f, 0.0f);
    option->getComponent<Transform>()->setSize(240.0f, 44.0f);
    option->addOption(L"Comfort", 10);
    option->addOption(L"Sport", 20);
    root->addChild(option);

    click(option);
    expect(option->getPopupMenu()->isOpen(), "option button should open its popup menu on activation");
    bool callbackCalled = false;
    option->setOnSelectedCallback([&callbackCalled](int id, const std::wstring&) { callbackCalled = id == 20; });
    auto secondItem = std::dynamic_pointer_cast<MRButton>(option->getPopupMenu()->m_children[2]);
    click(secondItem);
    expect(option->getSelectedId() == 20 && option->getSelectedText() == L"Sport", "option button should store the selected id and text");
    expect(callbackCalled, "option button should forward the selected item callback");
    expect(!option->getPopupMenu()->isOpen(), "selecting an option should close the popup menu");
}

void testMenuButton() {
    auto root = std::make_shared<UIWidget>(false);
    root->getComponent<Transform>()->setSize(640.0f, 360.0f);

    auto menuButton = MRMenuButton::create();
    menuButton->setText(L"Actions", "default");
    menuButton->getComponent<Transform>()->setPosition(20.0f, 20.0f, 0.0f);
    menuButton->getComponent<Transform>()->setSize(240.0f, 44.0f);
    menuButton->addMenuItem(L"Save", 1);
    root->addChild(menuButton);

    click(menuButton);
    expect(menuButton->getPopupMenu()->isOpen(), "menu button should open its popup menu on activation");
}

}  // namespace

int main() {
    testOptionButton();
    testMenuButton();

    if (g_failures != 0) {
        std::cerr << g_failures << " menu control test(s) failed\n";
        return 1;
    }
    std::cout << "All menu control tests passed\n";
    return 0;
}
