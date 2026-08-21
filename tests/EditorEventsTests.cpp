#include <iostream>
#include <string>

#include "EditorEvents.h"
#include "assets/AssetDatabase.h"

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

} // namespace

int main() {
    testEditorEventPayloads();
    testEditorEventConnectionsAreScoped();

    if (g_failures != 0) {
        std::cerr << g_failures << " EditorEvents test(s) failed\n";
        return 1;
    }

    std::cout << "All EditorEvents tests passed\n";
    return 0;
}
