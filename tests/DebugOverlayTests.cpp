#include <iostream>
#include <string>

#include "debug/DebugPlane.h"

using namespace morrow;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

void testDebugOverlayVisibilityState() {
    DebugPlane overlay;
    expect(!overlay.isVisible(), "debug overlay should be disabled by default");

    overlay.setVisible(true);
    expect(overlay.isVisible(), "setVisible(true) should enable the debug overlay");

    overlay.toggleVisible();
    expect(!overlay.isVisible(), "toggleVisible should disable an enabled overlay");

    overlay.toggleVisible();
    expect(overlay.isVisible(), "toggleVisible should enable a disabled overlay");
}

} // namespace

int main() {
    testDebugOverlayVisibilityState();

    if (g_failures != 0) {
        std::cerr << g_failures << " debug overlay test(s) failed\n";
        return 1;
    }

    std::cout << "All debug overlay tests passed\n";
    return 0;
}
