#include <iostream>
#include <string>

#include "ui/elements/MRCheckBox.h"
#include "ui/elements/MRRadioButton.h"
#include "ui/elements/MRToggle.h"

using namespace morrow;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

void click(const MRSelectableButtonSharedPtr& button) {
    button->onMouseDown();
    button->onMouseUp();
}

void testIndependentSelection() {
    auto first = MRCheckBox::create();
    auto second = MRCheckBox::create();

    click(first);
    expect(first->isChecked(), "clicking a checkbox should select it");
    expect(!second->isChecked(), "checkboxes should not affect each other");

    click(first);
    expect(!first->isChecked(), "clicking a selected checkbox should clear it");
}

void testToggleUpdatesBeforeClickCallback() {
    auto toggle = MRToggle::create();
    bool callbackObservedChecked = false;
    toggle->setOnClickCallback([&]() { callbackObservedChecked = toggle->isChecked(); });

    click(toggle);
    expect(toggle->isChecked(), "clicking a toggle should switch it on");
    expect(callbackObservedChecked, "the toggle state should update before the user click callback");
}

void testRadioGroupIsExclusive() {
    auto group = std::make_shared<MRRadioGroup>();
    auto first = MRRadioButton::create();
    auto second = MRRadioButton::create();
    first->setGroup(group);
    second->setGroup(group);

    click(first);
    expect(first->isChecked() && !second->isChecked(), "selecting the first radio button should clear its peers");

    click(second);
    expect(!first->isChecked() && second->isChecked(), "selecting another radio button should move the group selection");
    expect(group->getSelected() == second, "the radio group should report its selected button");

    click(second);
    expect(second->isChecked(), "clicking the selected radio button should not clear the group");
}

}  // namespace

int main() {
    testIndependentSelection();
    testToggleUpdatesBeforeClickCallback();
    testRadioGroupIsExclusive();

    if (g_failures != 0) {
        std::cerr << g_failures << " selection control test(s) failed\n";
        return 1;
    }
    std::cout << "All selection control tests passed\n";
    return 0;
}
