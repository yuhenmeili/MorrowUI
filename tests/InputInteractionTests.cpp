#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "platform/InputEventsManager.h"
#include "platform/Platform.h"
#include "platform/Window.h"
#include "ui/base/BaseButton.h"
#include "ui/base/TouchEvent.h"
#include "ui/base/Transform.h"
#include "ui/base/UIWidget.h"

using namespace morrow;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

class TestButton final : public BaseButton {
public:
    ButtonState state() const {
        return m_currentState;
    }

protected:
    void updateVisualState() override {
        ++visualUpdateCount;
    }

public:
    int visualUpdateCount = 0;
};

class TestPlatform final : public Platform {
public:
    TestPlatform() : m_inputManager(std::make_shared<InputEventsManager>()) {}

    bool beginFrame(FrameStateSharedPtr frameState) override {
        resolveInputTargets(frameState);
        return true;
    }
    void beginRenderPass(FrameStateSharedPtr) override {}
    void commitRenderPass(FrameStateSharedPtr) override {}
    void endFrame() override {}
    void terminate() override {}
    InputEventsManagerSharedPtr getInputManager() override { return m_inputManager; }

    void setWindow(const WindowSharedPtr& window) { m_window = window; }

private:
    InputEventsManagerSharedPtr m_inputManager;
};

void testScreenSpaceAABBUsesTouchCoordinates() {
    auto root = std::make_shared<UIWidget>(false);
    root->getTransform()->setSize(640.0f, 360.0f);

    auto child = std::make_shared<UIWidget>(false);
    child->getTransform()->setPosition(10.0f, 20.0f, 0.0f);
    child->getTransform()->setSize(100.0f, 40.0f);
    root->addChild(child);

    const auto bounds = child->getScreenSpaceAABB();
    expect(bounds.Min.x == 10.0f && bounds.Min.y == 20.0f &&
               bounds.Max.x == 110.0f && bounds.Max.y == 60.0f,
           "screen-space AABB should use top-left framebuffer coordinates");
    expect(bounds.Contains(60.0f, 40.0f),
           "TouchEvent coordinates should be directly usable for AABB hit testing");
}

void testPointerBoundaryEvents() {
    auto platform = std::make_shared<TestPlatform>();
    auto window = std::make_shared<Window>();
    window->getTransform()->setSize(640.0f, 360.0f);

    auto first = std::make_shared<TestButton>();
    first->getTransform()->setPosition(10.0f, 20.0f, 0.0f);
    first->getTransform()->setSize(100.0f, 40.0f);
    window->addChild(first);

    auto second = std::make_shared<TestButton>();
    second->getTransform()->setPosition(120.0f, 20.0f, 0.0f);
    second->getTransform()->setSize(100.0f, 40.0f);
    window->addChild(second);
    platform->setWindow(window);

    auto frameState = std::make_shared<FrameState>();
    frameState->inputEventsManager = platform->getInputManager();
    size_t observedResolvedEventCount = 0;
    frameState->inputEventsManager->getResolvedInputEventsDispatcher().add(
        [&](std::vector<TouchEvent>& events) {
            observedResolvedEventCount = events.size();
        });

    TouchEvent move;
    move.touchID = -1;
    move.positionX = 60.0f;
    move.positionY = 40.0f;
    move.eventType = TOUCH_EVENT_TYPE_MOVE;
    move.deviceType = TOUCH_DEVICE_TYPE_MOUSE;
    frameState->inputEventsManager->getInputEvents().push_back(move);
    platform->beginFrame(frameState);

    auto& firstEvents = frameState->inputEventsManager->getInputEvents();
    expect(firstEvents.size() == 2 &&
               firstEvents[0].eventType == TOUCH_EVENT_TYPE_POINTER_ENTER &&
               firstEvents[0].target == first &&
               firstEvents[1].eventType == TOUCH_EVENT_TYPE_MOVE,
           "first hover should emit POINTER_ENTER before MOVE");
    expect(observedResolvedEventCount == 2,
           "resolved input observers should receive synthesized pointer events");

    firstEvents.clear();
    move.positionX = 170.0f;
    firstEvents.push_back(move);
    platform->beginFrame(frameState);

    auto& transitionEvents = frameState->inputEventsManager->getInputEvents();
    expect(transitionEvents.size() == 3 &&
               transitionEvents[0].eventType == TOUCH_EVENT_TYPE_POINTER_LEAVE &&
               transitionEvents[0].target == first &&
               transitionEvents[1].eventType == TOUCH_EVENT_TYPE_POINTER_ENTER &&
               transitionEvents[1].target == second &&
               transitionEvents[2].eventType == TOUCH_EVENT_TYPE_MOVE,
           "hover target change should emit LEAVE, ENTER, then MOVE");
    expect(observedResolvedEventCount == 3,
           "resolved input observers should receive the full hover transition");
}

void testButtonHoverUsesPointerEvents() {
    auto root = std::make_shared<UIWidget>(false);
    root->getTransform()->setSize(640.0f, 360.0f);

    auto button = std::make_shared<TestButton>();
    button->getTransform()->setPosition(10.0f, 20.0f, 0.0f);
    button->getTransform()->setSize(100.0f, 40.0f);
    root->addChild(button);

    TouchEvent enter;
    enter.touchID = -1;
    enter.positionX = 60.0f;
    enter.positionY = 40.0f;
    enter.eventType = TOUCH_EVENT_TYPE_POINTER_ENTER;
    enter.target = button;
    button->dispatchTouchEvent(enter);

    expect(button->state() == ButtonState::HOVER,
           "button should enter hover using TouchEvent coordinates directly");

    TouchEvent leave = enter;
    leave.eventType = TOUCH_EVENT_TYPE_POINTER_LEAVE;
    button->dispatchTouchEvent(leave);

    expect(button->state() == ButtonState::NORMAL,
           "button should leave hover when the pointer exits its screen-space AABB");
    expect(button->visualUpdateCount == 2,
           "hover enter and leave should each update the visual state");
}

}  // namespace

int main() {
    testScreenSpaceAABBUsesTouchCoordinates();
    testPointerBoundaryEvents();
    testButtonHoverUsesPointerEvents();

    if (g_failures != 0) {
        std::cerr << g_failures << " input interaction test(s) failed\n";
        return 1;
    }
    std::cout << "All input interaction tests passed\n";
    return 0;
}
