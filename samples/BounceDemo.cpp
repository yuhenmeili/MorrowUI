#include "Engine.h"
#include "Texture.h"
#include "elements/MRAnchorPointScale.h"
#include "base/Transform.h"
#include "elements/MRBounce.h"
//
// Created by lance on 2025/10/4.
//
using namespace morrow;

int main() {
    EngineSharedPtr engine = std::make_shared<Engine>();

    auto window = engine->getWindow();
    window->setClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    auto texture = Texture::create(ImageType::IMAGE);
    texture->setImageUrl("assets/textures/bounce/aeb_r.png");

    auto widget = MRBounce::create();
    auto transform = widget->getComponent<Transform>();
    transform->setPosition(100, 100, 0);
    transform->setSize(100, 100);

    auto meshRenderer = widget->getComponent<MeshRenderer>();
    auto material = meshRenderer->getMaterial();
    material->setTexture("texture", texture);
    material->setFloat("bounceTimes", 3.0f);
    material->setFloat("scaleRange", 0.5f);
    material->setFloat("duration", 1.0f);
    material->setVector("meshCenter", transform->getCenter());

    auto timeTween = Tween::create(0.0f, 1.0f, 1.0f);
    timeTween->setEase(EaseType::Linear)
             .onUpdate([material](float value) {
                 material->setFloat("timeDelta", value);
             })
             .onComplete([timeTween]() {
                 timeTween->restart();
             });

    timeTween->play();
    TweenManager::getInstance().addTween(timeTween);

    window->addChild(widget);

    LOG_I("start render");
    engine->render();
    return 0;
}
