#include "Engine.h"
#include "elements/MRGearsOpening.h"
#include "base/Transform.h"
#include "../src/ui/helpers/Tween.h"
//
// Created by lance on 2023/3/11.
//
using namespace morrow;

int main() {
    EngineSharedPtr engine = std::make_shared<Engine>();
    auto window = engine->getWindow();
    window->setClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    auto widget = MRGearsOpening::create();
    auto transform = widget->getComponent<Transform>();
    transform->setPosition(100.0f, 100.0f, 0.0f);
    transform->setSize(198.0f, 708.0f);

    auto texture = Texture::create(ImageType::IMAGE);
    texture->setImageUrl("../assets/textures/gearBG.png");

    auto meshRenderer = widget->getComponent<MeshRenderer>();
    auto material = meshRenderer->getMaterial();
    material->setTexture("texture", texture);

    widget->initialize();
    window->addChild(widget);
    LOG_I("start render");
    engine->render();
    return 0;
}
