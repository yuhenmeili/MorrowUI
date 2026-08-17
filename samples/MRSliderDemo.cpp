//
// Created by lance on 2026/8/17.
//

#include "Engine.h"
#include "base/Transform.h"
#include "elements/MRSlider.h"

using namespace morrow;

int main() {
    auto engine = std::make_shared<Engine>();
    auto window = engine->getWindow();
    window->setClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // 横向滑块（默认左→右）
    auto hSlider = MRSlider::create();
    hSlider->getComponent<Transform>()->setPosition(100.0f, 150.0f, 0.0f);
    hSlider->getComponent<Transform>()->setSize(300.0f, 24.0f);
    hSlider->setTrackColor(0.85f, 0.85f, 0.85f, 1.0f);
    hSlider->setFillColor(0.2f, 0.6f, 0.9f, 1.0f);
    hSlider->setRounding(12.0f);
    hSlider->setThumbSize(Vector2(32.0f, 32.0f));
    hSlider->setThumbColor(0.1f, 0.4f, 0.8f, 1.0f);
    hSlider->setThumbRounding(16.0f);
    hSlider->setValue(0.3f);
    hSlider->setOnValueChangedCallback([](float value) {
        LOG_I("horizontal slider value = {}", value);
    });
    window->addChild(hSlider);

    // 纵向滑块（下→上）
    auto vSlider = MRSlider::create();
    vSlider->getComponent<Transform>()->setPosition(500.0f, 100.0f, 0.0f);
    vSlider->getComponent<Transform>()->setSize(24.0f, 200.0f);
    vSlider->setDirection(ProgressDirection::BottomToTop);
    vSlider->setTrackColor(0.85f, 0.85f, 0.85f, 1.0f);
    vSlider->setFillColor(0.3f, 0.8f, 0.4f, 1.0f);
    vSlider->setRounding(12.0f);
    vSlider->setThumbSize(Vector2(32.0f, 32.0f));
    vSlider->setThumbColor(0.1f, 0.6f, 0.3f, 1.0f);
    vSlider->setThumbRounding(16.0f);
    vSlider->setValue(0.6f);
    vSlider->setOnValueChangedCallback([](float value) {
        LOG_I("vertical slider value = {}", value);
    });
    window->addChild(vSlider);

    LOG_I("start render slider demo (drag the thumb with mouse)");
    engine->render();
    return 0;
}
