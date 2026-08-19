#include <iostream>
#include <memory>
#include <string>

#include "ui/base/TouchEvent.h"
#include "ui/base/Transform.h"
#include "ui/elements/MRColor.h"
#include "ui/elements/MRScrollContainer.h"

using namespace morrow;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

void testScrollRangeAndSynchronization() {
    auto scroll = MRScrollContainer::create();
    scroll->getComponent<Transform>()->setSize(320.0f, 120.0f);

    for (int index = 0; index < 5; ++index) {
        auto item = MRColor::create();
        item->getComponent<Transform>()->setPosition(0.0f, static_cast<float>(index) * 40.0f, 0.0f);
        item->getComponent<Transform>()->setSize(280.0f, 32.0f);
        scroll->addScrollChild(item);
    }

    scroll->setScrollOffset(0.0f);

    expect(scroll->getMaxScrollOffset() > 0.0f, "content taller than the viewport should produce a scroll range");
    expect(scroll->getVerticalScrollBar()->getPageRatio() < 1.0f, "scrollbar page ratio should reflect the content-to-viewport ratio");

    TouchEvent wheel;
    wheel.eventType = TOUCH_EVENT_TYPE_WHEEL;
    wheel.wheelDeltaY = -1.0f;
    scroll->getContent()->dispatchTouchEvent(wheel);
    expect(scroll->getScrollOffset() > 0.0f, "wheel input should advance the scroll container");

    scroll->setScrollOffset(10000.0f);
    expect(scroll->getScrollOffset() == scroll->getMaxScrollOffset(), "scroll offset should clamp to the maximum range");
    expect(scroll->getVerticalScrollBar()->getValue() == 1.0f, "scrollbar value should follow the clamped scroll offset");
}

}  // namespace

int main() {
    testScrollRangeAndSynchronization();

    if (g_failures != 0) {
        std::cerr << g_failures << " scroll control test(s) failed\n";
        return 1;
    }
    std::cout << "All scroll control tests passed\n";
    return 0;
}
