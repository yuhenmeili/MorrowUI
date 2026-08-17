#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "platform/InputEventsManager.h"
#include "platform/InputProvider.h"
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

class MoveInputProvider final : public IInputProvider {
public:
    void poll(std::vector<TouchEvent>& outEvents) override {
        TouchEvent event;
        event.touchID = -1;
        event.positionX = 320.0f;
        event.positionY = 180.0f;
        event.eventType = TOUCH_EVENT_TYPE_MOVE;
        event.deviceType = TOUCH_DEVICE_TYPE_MOUSE;
        outEvents.push_back(event);
    }
};

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

void testInputPollNotifiesObservers() {
    InputEventsManager manager;
    manager.setInputProvider(std::make_shared<MoveInputProvider>());

    int notificationCount = 0;
    size_t observedEventCount = 0;
    manager.getInputEventsDispatcher().add(
        [&](std::vector<TouchEvent>& events) {
            ++notificationCount;
            observedEventCount = events.size();
        });

    manager.poll();

    expect(notificationCount == 1, "poll should notify input observers once");
    expect(observedEventCount == 1, "observer should receive the polled move event");
}

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

void testButtonHoverUsesTouchCoordinates() {
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
    enter.eventType = TOUCH_EVENT_TYPE_MOVE;
    enter.target = button;
    button->dispatchTouchEvent(enter);

    expect(button->state() == ButtonState::HOVER,
           "button should enter hover using TouchEvent coordinates directly");

    TouchEvent leave = enter;
    leave.positionX = 200.0f;
    leave.positionY = 200.0f;
    leave.target.reset();
    button->dispatchTouchEvent(leave);

    expect(button->state() == ButtonState::NORMAL,
           "button should leave hover when the pointer exits its screen-space AABB");
    expect(button->visualUpdateCount == 2,
           "hover enter and leave should each update the visual state");
}

}  // namespace

int main() {
    testInputPollNotifiesObservers();
    testScreenSpaceAABBUsesTouchCoordinates();
    testButtonHoverUsesTouchCoordinates();

    if (g_failures != 0) {
        std::cerr << g_failures << " input interaction test(s) failed\n";
        return 1;
    }
    std::cout << "All input interaction tests passed\n";
    return 0;
}
