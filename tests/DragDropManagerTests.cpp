#include <iostream>
#include <string>

#include "ui/layout/DragDropManager.h"

using namespace morrow;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

void testDragLifecycle() {
    DragDropManager manager;
    DragPayload payload;
    payload.type = "editor/dock-panel";
    payload.id = "filesystem";

    int started = 0;
    int moved = 0;
    int dropped = 0;
    int cancelled = 0;
    auto startedConnection = manager.onDragStarted.connect(
        [&](const DragState& state) {
            ++started;
            expect(state.payload.id == "filesystem",
                   "drag start should preserve payload id");
        });
    auto movedConnection = manager.onDragMoved.connect(
        [&](const DragState& state) {
            ++moved;
            expect(state.x == 20.0f && state.y == 30.0f,
                   "drag move should update pointer position");
        });
    auto droppedConnection = manager.onDragDropped.connect(
        [&](const DragState& state) {
            ++dropped;
            expect(state.active, "drop event should observe an active drag");
        });
    auto cancelledConnection = manager.onDragCancelled.connect(
        [&](const DragState&) { ++cancelled; });

    expect(manager.begin(payload, 10.0f, 15.0f),
           "drag should begin with a valid payload");
    expect(!manager.begin(payload, 1.0f, 1.0f),
           "a second drag should not replace an active drag");
    expect(manager.update(20.0f, 30.0f),
           "active drag should accept movement");
    expect(manager.drop(25.0f, 35.0f),
           "active drag should accept a drop");
    expect(!manager.isActive(), "drop should clear active drag state");
    expect(started == 1 && moved == 1 && dropped == 1 && cancelled == 0,
           "drag lifecycle events should fire exactly once");

    expect(manager.begin(payload, 1.0f, 2.0f),
           "manager should allow a new drag after drop");
    expect(manager.cancel(), "active drag should be cancellable");
    expect(!manager.isActive(), "cancel should clear active drag state");
    expect(cancelled == 1, "cancel event should fire exactly once");
}

}  // namespace

int main() {
    testDragLifecycle();
    if (g_failures != 0) {
        std::cerr << g_failures << " DragDropManager test(s) failed\n";
        return 1;
    }
    std::cout << "All DragDropManager tests passed\n";
    return 0;
}
