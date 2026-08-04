#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "FrameState.h"
#include "GlobalObject.h"
#include "RenderingThread.h"
#include "ui/helpers/Tween.h"

using namespace morrow;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

void testRenderRequestConsumption() {
    RenderingThread renderingThread;

    expect(renderingThread.consumeRenderRequest(),
           "the first frame should be requested by default");
    expect(!renderingThread.consumeRenderRequest(),
           "a consumed request should not be returned twice");

    renderingThread.requestRender();
    expect(renderingThread.consumeRenderRequest(),
           "requestRender should publish one render request");

    std::thread requester([&renderingThread]() {
        renderingThread.requestRender();
    });
    requester.join();
    expect(renderingThread.consumeRenderRequest(),
           "render requests should be safe to publish from another thread");
}

void testTweenKeepsRequestingFramesWhileActive() {
    const auto renderingThread =
        GlobalObject::getInstance().getRenderingThread();
    renderingThread->resetRenderStatus();

    auto tween = Tween::create(0.0f, 1.0f, 1.0f);
    tween->play();
    expect(renderingThread->consumeRenderRequest(),
           "starting a tween should request its first frame");

    TweenManager::getInstance().addTween(tween);
    expect(renderingThread->consumeRenderRequest(),
           "registering a tween should wake an idle engine");

    auto frameState = std::make_shared<FrameState>();
    frameState->deltaTime = 0.25;
    TweenManager::getInstance().update(frameState);
    expect(renderingThread->consumeRenderRequest(),
           "an active tween should request the next frame");

    frameState->deltaTime = 1.0;
    TweenManager::getInstance().update(frameState);
    expect(!renderingThread->consumeRenderRequest(),
           "a completed tween should stop requesting frames");

    TweenManager::getInstance().removeAllTweens();
}

} // namespace

int main() {
    testRenderRequestConsumption();
    testTweenKeepsRequestingFramesWhileActive();

    if (g_failures != 0) {
        std::cerr << g_failures << " request-render test(s) failed\n";
        return 1;
    }
    std::cout << "All request-render tests passed\n";
    return 0;
}
