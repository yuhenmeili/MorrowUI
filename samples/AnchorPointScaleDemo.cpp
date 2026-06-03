#include "Engine.h"
#include "Texture.h"
#include "Vector2.h"
#include "base/MeshRenderer.h"
#include "base/Transform.h"
#include "elements/MRAnchorPointScale.h"
//
// Created by lance on 2025/10/3.
//
using namespace morrow;

int main()
{
    EngineSharedPtr engine = std::make_shared<Engine>();

    auto window = engine->getWindow();
    window->setClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    auto textAtlas = Texture::create(ImageType::IMAGE);
    textAtlas->setImageUrl("assets/textures/img.jpg");

    auto widget = MRAnchorPointScale::create();
    auto transform = widget->getComponent<Transform>();
    transform->setPosition(100, 100, 0);
    transform->setSize(100, 100);

    auto meshRenderer = widget->getComponent<MeshRenderer>();
    auto material = meshRenderer->getMaterial();
    material->setTexture("texture", textAtlas);

    transform->setPivot(0.0f, 1.0f, 0.0f);
    auto scaleTween = Tween::create(0.0f, 1.0f, 1.5f);
    scaleTween->setEase(EaseType::Linear)
                .onUpdate([transform](float value) {
                    // 更新Transform的缩放
                    LOG_I("current scale {}", value);
                    transform->setScale(value, value, 1.0f);
                })
                .onComplete([]() {
                    LOG_I("缩放动画完成!");
                });

    scaleTween->play();
    // 添加到Tween管理器
    TweenManager::getInstance().addTween(scaleTween);

    window->addChild(widget);

    LOG_I("start render");
    engine->render();
    return 0;
}
