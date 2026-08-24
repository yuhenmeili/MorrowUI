#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

#include "EditorEvents.h"
#include "assets/AssetDatabase.h"
#include "ui/DockLayout.h"

using namespace morrow::editor;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

void testEditorEventPayloads() {
    EditorEvents events;
    SelectionState selection;
    selection.nodeIds = {"root", "button"};
    AssetDatabase assets;

    const SelectionState* observedSelection = nullptr;
    const AssetDatabase* observedAssets = nullptr;
    int runtimeRebuildCount = 0;

    auto selectionConnection =
        events.onSelectionChanged.connect(
            [&](const SelectionState& value) { observedSelection = &value; });
    auto runtimeConnection =
        events.onRuntimeRebuilt.connect([&]() { ++runtimeRebuildCount; });
    auto assetConnection =
        events.onAssetDatabaseChanged.connect(
            [&](const AssetDatabase& value) { observedAssets = &value; });

    events.onSelectionChanged.notify(selection);
    events.onRuntimeRebuilt.notify();
    events.onAssetDatabaseChanged.notify(assets);

    expect(observedSelection == &selection,
           "selection event should forward the current editor selection");
    expect(runtimeRebuildCount == 1,
           "runtime rebuilt event should notify connected editor observers");
    expect(observedAssets == &assets,
           "asset database event should forward the owning editor database");
}

void testEditorEventConnectionsAreScoped() {
    EditorEvents events;
    int calls = 0;

    {
        auto connection =
            events.onRuntimeRebuilt.connect([&]() { ++calls; });
        events.onRuntimeRebuilt.notify();
    }
    events.onRuntimeRebuilt.notify();

    expect(calls == 1,
           "destroying an editor event connection should stop later notifications");
}

void testDockLayoutPersistsSplitRatios() {
    auto layout = DockLayout::defaultLayout(1920.0f, 1080.0f);
    layout.findSplit("workspace")->ratio = 0.71f;
    layout.findSplit("left")->ratio = 0.22f;
    layout.findSplit("center")->ratio = 0.63f;
    layout.findTabs("left_dock")->active = "filesystem";
    layout.findTabs("left_dock")->panels = {"filesystem", "scene_tree"};

    const auto path =
        std::filesystem::temp_directory_path() /
        "morrow_editor_dock_layout_test.layout";
    std::error_code filesystemError;
    std::filesystem::remove(path, filesystemError);

    std::string error;
    expect(layout.save(path, error),
           "dock layout should save split ratios");

    DockLayout loaded;
    expect(loaded.load(path, error),
           "dock layout should load saved split ratios");
    const auto near = [](float left, float right) {
        return std::abs(left - right) < 0.0001f;
    };
    expect(loaded.findSplit("workspace") &&
               near(loaded.findSplit("workspace")->ratio, 0.71f),
           "workspace split ratio should survive persistence");
    expect(loaded.findSplit("left") &&
               near(loaded.findSplit("left")->ratio, 0.22f),
           "left split ratio should survive persistence");
    expect(loaded.findSplit("center") &&
               near(loaded.findSplit("center")->ratio, 0.63f),
           "center split ratio should survive persistence");
    expect(loaded.findTabs("left_dock") &&
               loaded.findTabs("left_dock")->active == "filesystem",
           "active tab should survive persistence");
    expect(loaded.findTabs("left_dock") &&
               loaded.findTabs("left_dock")->panels ==
                   std::vector<std::string>({"filesystem", "scene_tree"}),
           "tab order should survive persistence");

    std::filesystem::remove(path, filesystemError);
}

} // namespace

int main() {
    testEditorEventPayloads();
    testEditorEventConnectionsAreScoped();
    testDockLayoutPersistsSplitRatios();

    if (g_failures != 0) {
        std::cerr << g_failures << " EditorEvents test(s) failed\n";
        return 1;
    }

    std::cout << "All EditorEvents tests passed\n";
    return 0;
}
