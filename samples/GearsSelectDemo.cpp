#include "Engine.h"
#include "elements/MRGearsSelect.h"
#include "base/Transform.h"
//
// Created by lance on 2023/3/11.
//
using namespace morrow;
using namespace morrow::Math;

int main() {
    EngineSharedPtr engine = std::make_shared<Engine>();

    auto window = engine->getWindow();
    window->setClearColor(0.7, 0.5, 0.3, 0.5);

    auto widget = MRGearsSelect::create();
    auto transform = widget->getComponent<Transform>();
    transform->setPosition(Vector3(100.0f, 100.0f, 0.0f));
    transform->setSize(Vector3(10.0f, 10.0f, 0.0f));

    widget->initialize();
    window->addChild(widget);
    engine->render();
    return 0;
};
