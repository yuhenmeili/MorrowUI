#include <iostream>
#include <memory>
#include <string>

#include "ui/base/Transform.h"
#include "ui/layout/MRTabContainer.h"

using namespace morrow;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

void testTabSelectionAndOrdering() {
    auto tabs = MRTabContainer::create();
    tabs->getTransform()->setSize(400.0f, 240.0f);
    auto scene = std::make_shared<UIWidget>(false);
    auto fileSystem = std::make_shared<UIWidget>(false);
    auto output = std::make_shared<UIWidget>(false);
    int selectionChanges = 0;
    auto connection = tabs->events().onCurrentTabChanged.connect(
        [&selectionChanges](MRTabContainer&, const std::string&) {
            ++selectionChanges;
        });

    expect(tabs->addTab("scene", L"Scene", scene),
           "first tab should be added");
    expect(tabs->addTab("filesystem", L"FileSystem", fileSystem),
           "second tab should be added");
    expect(tabs->addTab("output", L"Output", output),
           "third tab should be added");
    expect(tabs->currentTabId() == "scene",
           "first tab should be active by default");
    expect(scene->getVisible() && !fileSystem->getVisible(),
           "only the active tab content should be visible");

    expect(tabs->selectTab("filesystem"),
           "existing tab should be selectable");
    expect(tabs->currentTabId() == "filesystem" &&
               fileSystem->getVisible() && !scene->getVisible(),
           "selecting a tab should update content visibility");
    expect(selectionChanges == 1,
           "selection event should fire once for a changed tab");

    expect(tabs->moveTab(2, 0), "tabs should be reorderable");
    expect(tabs->tabs()[0].id == "output",
           "moving a tab should update tab order");
    expect(tabs->currentTabId() == "filesystem",
           "reordering should preserve the active tab");
}

}  // namespace

int main() {
    testTabSelectionAndOrdering();
    if (g_failures != 0) {
        std::cerr << g_failures << " TabContainer test(s) failed\n";
        return 1;
    }
    std::cout << "All TabContainer tests passed\n";
    return 0;
}
