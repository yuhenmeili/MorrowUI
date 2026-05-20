#include "Engine.h"
#include "Texture.h"
#include "base/Transform.h"
#include "../src/ui/helpers/Tween.h"
#include "elements/MRBrakePedal.h"
//
// Created by lance on 2025/10/4.
//
using namespace morrow;

int main()
{
    EngineSharedPtr engine = std::make_shared<Engine>();

    auto window = engine->getWindow();
    window->setClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    auto brakePedalTexture = Texture::create(ImageType::IMAGE);
    brakePedalTexture->setImageUrl("../assets/textures/brake_pedal/brakePedal.png");

    auto whiteTexture = Texture::create(ImageType::IMAGE);
    whiteTexture->setImageUrl("../assets/textures/brake_pedal/whiteCircle.png");

    auto grayTexture = Texture::create(ImageType::IMAGE);
    grayTexture->setImageUrl("../assets/textures/brake_pedal/grayCircle.png");

    auto widget = MRBrakePedal::create();
    auto transform = widget->getComponent<Transform>();
    transform->setPosition(100, 100, 0);
    transform->setSize(256, 256);

    auto meshRenderer = widget->getComponent<MeshRenderer>();
    auto material = meshRenderer->getMaterial();
    material->setTexture("brakePedalTexture", brakePedalTexture);
    material->setTexture("grayTexture", grayTexture);
    material->setTexture("whiteTexture", whiteTexture);
    material->setFloat("duration", 1.0f);

    auto timeTween = Tween::create(0.0f, 1.0f, 1.0f);
    timeTween->setEase(EaseType::Linear)
                .onUpdate([material](float value) {
                    material->setFloat("timeDelta", value);
                })
                .onComplete([]() {
                    LOG_I("The zooming animation is complete!");
                });

    timeTween->play();
    TweenManager::getInstance().addTween(timeTween);

    window->addChild(widget);

    LOG_I("start render");
    engine->render();
    return 0;
}