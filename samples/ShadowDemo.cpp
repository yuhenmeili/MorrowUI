//
// Created by 0060328 on 25-10-14.
//

#include "Engine.h"
#include "base/Shadow.h"
#include "elements/MRImage.h"
#include "Window.h"
#include "base/Transform.h"

using namespace morrow;

int main() {
    EngineSharedPtr engine = std::make_shared<Engine>();

    auto window = engine->getWindow();
    window->setClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    auto image = MRImage::create();
    auto transform = image->getComponent<Transform>();
    transform->setPosition(100, 100, 0);
    transform->setSize(100, 100);

    auto meshRenderer = image->getComponent<MeshRenderer>();
    auto material = meshRenderer->getMaterial();
    material->setShader("default_color");
    material->setVector("color", Vector4(1.0f, 0.0f, 0.0f, 1.0f));

    // 添加阴影组件
    auto shadow = image->addComponent<Shadow>();
    shadow->setShadowOffset(Vector2(5.0f, 5.0f));
    shadow->setShadowColor(Vector4(0.0f, 0.0f, 0.0f, 0.5f));

    // 将按钮添加到窗口
    window->addChild(image);

    engine->render();
    return 0;
}