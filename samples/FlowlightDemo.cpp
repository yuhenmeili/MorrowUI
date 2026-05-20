//
// Created by lance on 2023/2/24.
//

#include "Engine.h"
#include "elements/MRFlowingLight.h"
#include "base/Transform.h"
#include "../src/ui/helpers/Tween.h"

using namespace morrow;
using namespace morrow::Math;

int main() {
    EngineSharedPtr engine = std::make_shared<Engine>();

    auto window = engine->getWindow();
    window->setClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    auto widget = MRFlowingLight::create();
    auto transform = widget->getComponent<Transform>();
    transform->setPosition(100, 100, 0);
    transform->setSize(100, 100);

    widget->initialize();
    window->addChild(widget);

    engine->render();
    return 0;
}
