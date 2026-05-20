#include "Engine.h"
#include "elements/MRGearsShine.h"
#include "base/Transform.h"
//
// Created by lance on 2025/10/7.
//
using namespace morrow;

int main() {
    EngineSharedPtr engine = std::make_shared<Engine>();
    auto window = engine->getWindow();
    window->setClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    auto widget = MRGearsShine::create();
    auto transform = widget->getComponent<Transform>();
    transform->setPosition(100.0f, 100.0f, 0.0f);
    transform->setSize(144.0f, 684.0f);

    widget->initialize();
    window->addChild(widget);
    LOG_I("start render");
    engine->render();
    return 0;
}