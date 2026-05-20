//
// Created by lance on 2025/10/6.
//
#include "Engine.h"
#include "elements/MRGearsOpening3D.h"
#include "base/Transform.h"
#include "../src/ui/helpers/Tween.h"
using namespace morrow;

int main() {
    EngineSharedPtr engine = std::make_shared<Engine>();
    auto window = engine->getWindow();
    window->setClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    auto widget = MRGearsOpening3D::create();
    auto transform = widget->getComponent<Transform>();
    transform->setPosition(100.0f, 100.0f, 0.0f);
    transform->setSize(198.0f, 708.0f);

    widget->initialize();
    window->addChild(widget);
    LOG_I("start render gears opening 3D");
    engine->render();
    return 0;
}