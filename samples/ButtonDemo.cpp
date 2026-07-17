//
// Created by lance on 2023/1/31.
//

#include "Engine.h"
#include "FontManager.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "layout/CenterContainer.h"
#include "layout/HBoxContainer.h"

using namespace morrow;
using namespace morrow::Math;

int main()
{
    auto engine = std::make_shared<Engine>();

    auto window = engine->getWindow();
    window->setClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    FontInfo fontInfo = {
        .name = "default",
        .path = "assets/fonts/MorrowSansCN1.1-Regular.otf",
        .fontSize = 32.0f
    };
    engine->addFonts({fontInfo});

    auto button1 = MRButton::create();
    button1->setText(L"Button1", "default");
    auto tex = Texture::create();
    tex->setImageUrl("assets/textures/img.jpg");
    button1->setBackgroundImage(tex);          // 设置底图


    auto transform = button1->getComponent<Transform>();
    transform->setSize(100.0f, 100.0f);
    // window->addChild(button1);

    auto button2 = MRButton::create();
    button2->setText(L"Button2", "default");
    button2->setBackgroundColor(1.0, 0.0f, 0.0f, 1.0f);
    auto transform2 = button2->getComponent<Transform>();
    transform2->setSize(200.0f, 200.0f);
    // window->addChild(button2);

    auto vbox = std::make_shared<HBoxContainer>();
    vbox->getComponent<Transform>()->setPosition(100, 100, 0);
    vbox->getComponent<Transform>()->setSize(200, 400);  // 容器尺寸，可选
    vbox->setSpacing(8);
    vbox->addChild(button1);  // 先加在上
    vbox->addChild(button2);  // 后加在下

    // auto center = std::make_shared<CenterContainer>();
    // center->getComponent<Transform>()->setSize(200, 200);  // 必须设容器尺寸
    // center->addChild(button1);  // 通常只加一个子节点，会被居中

    // auto margin = std::make_shared<MarginContainer>();
    // margin->getComponent<Transform>()->setPosition(0, 0, 0);
    // margin->getComponent<Transform>()->setSize(200, 200);
    // margin->setMargin(20, 10, 20, 10);   // 左 20、上 10、右 20、下 10
    // // 或 margin->setMarginAll(16);
    // margin->addChild(button1);  // 子节点会被放在 (20,10)，尺寸为 (360, 280)

    window->addChild(vbox);
    // window->addChild(center);
    // window->addChild(margin);
    engine->render();
    return 0;
}
