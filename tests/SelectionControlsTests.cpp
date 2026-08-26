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

MRColorSharedPtr indicatorFill(const MRSelectableButtonSharedPtr& button) {
    for (const auto& child : button->m_children) {
        const auto indicator = std::dynamic_pointer_cast<MRColor>(child);
        if (!indicator)
            continue;
        for (const auto& indicatorChild : indicator->m_children) {
            if (const auto fill = std::dynamic_pointer_cast<MRColor>(indicatorChild))
                return fill;
        }
    }
    return nullptr;
}

void testIndependentSelection() {
    auto first = MRCheckBox::create();
    auto second = MRCheckBox::create();
    const auto fill = indicatorFill(first);

    expect(fill && !fill->getVisible(), "an unchecked checkbox should hide its inner fill");

    click(first);
    expect(first->isChecked(), "clicking a checkbox should select it");
    expect(!second->isChecked(), "checkboxes should not affect each other");
    expect(fill && fill->getVisible(), "a checked checkbox should show its inner fill");

    click(first);
    expect(!first->isChecked(), "clicking a selected checkbox should clear it");
    expect(fill && !fill->getVisible(), "clearing a checkbox should hide its inner fill");
}

void testToggleUpdatesBeforeClickCallback() {
    auto toggle = MRToggle::create();
    bool callbackObservedChecked = false;
    auto clickConnection = toggle->events().onClicked.connect(
        [&](BaseButton&) { callbackObservedChecked = toggle->isChecked(); });

    click(toggle);
    expect(toggle->isChecked(), "clicking a toggle should switch it on");
    expect(callbackObservedChecked, "the toggle state should update before the user click callback");
}

void testButtonSupportsMultipleObserversAndRemoval() {
    auto button = MRCheckBox::create();
    int firstCalls = 0;
    int secondCalls = 0;
    auto firstConnection =
        button->events().onClicked.connect([&](BaseButton&) { ++firstCalls; });
    auto secondConnection =
        button->events().onClicked.connect([&](BaseButton&) { ++secondCalls; });

    click(button);
    firstConnection.disconnect();
    click(button);

    expect(firstCalls == 1, "disconnect should remove only the owning button observer");
    expect(secondCalls == 2, "multiple button observers should be notified independently");
}

void testRadioGroupIsExclusive() {
    auto group = std::make_shared<MRRadioGroup>();
    auto first = MRRadioButton::create();
    auto second = MRRadioButton::create();
    const auto firstFill = indicatorFill(first);
    first->setGroup(group);
    second->setGroup(group);

    click(first);
    expect(first->isChecked() && !second->isChecked(), "selecting the first radio button should clear its peers");
    expect(firstFill && firstFill->getVisible(), "a selected radio button should show its inner fill");

    click(second);
    expect(!first->isChecked() && second->isChecked(), "selecting another radio button should move the group selection");
    expect(group->getSelected() == second, "the radio group should report its selected button");
    expect(firstFill && !firstFill->getVisible(), "a cleared radio button should hide its inner fill");

    click(second);
    expect(second->isChecked(), "clicking the selected radio button should not clear the group");
}

void testRadioGroupContainerTracksChildren() {
    auto group = MRRadioGroup::create();
    auto first = MRRadioButton::create();
    auto second = MRRadioButton::create();
    group->addChild(first);
    group->addChild(second);

    expect(first->getGroup() == group && second->getGroup() == group, "a radio group node should automatically group direct radio button children");

    click(first);
    click(second);
    expect(!first->isChecked() && second->isChecked(), "radio button children should remain mutually exclusive");

    auto otherParent = std::make_shared<UIWidget>(false);
    otherParent->addChild(second);
    expect(!second->getGroup(), "moving a radio button out of a group node should detach it from the group");
}

}  // namespace

int main() {
    testIndependentSelection();
    testToggleUpdatesBeforeClickCallback();
    testButtonSupportsMultipleObserversAndRemoval();
    testRadioGroupIsExclusive();
    testRadioGroupContainerTracksChildren();

    if (g_failures != 0) {
        std::cerr << g_failures << " selection control test(s) failed\n";
        return 1;
    }
    std::cout << "All selection control tests passed\n";
    return 0;
}
