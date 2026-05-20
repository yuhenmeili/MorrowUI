//
// Created by user on 2025/10/11.
//

#include "Engine.h"
#include "FontManager.h"
#include "elements/MRImage.h"
#include "elements/MRLabel.h"
#include "base/Transform.h"

using namespace morrow;

int main() {
    // 创建引擎和窗口
    EngineSharedPtr engine = std::make_shared<morrow::Engine>();
    auto window = engine->getWindow();
    window->setClearColor(0.8f, 0.8f, 0.8f, 1.0f);

    FontInfo fontInfo = {
        .name = "default",
        .path = "../assets/fonts/MorrowSansCN1.1-Regular.otf",
        .fontSize = 32.0f
    };
    engine->addFonts({fontInfo});

    auto image = MRImage::create();
    auto imageTransform = image->getComponent<Transform>();
    imageTransform->setPosition(100, 100, 0);
    imageTransform->setSize(512, 512);

    auto imageRender = image->getComponent<MeshRenderer>();
    auto imageMaterial = imageRender->getMaterial();
    imageMaterial->setShader("default_color");
    imageMaterial->setVector("color", Vector4(1.0f, 1.0f, 0.0f, 1.0f));

    window->addChild(image);

    // 创建文字渲染器并演示对齐功能
    auto textRenderer = std::make_shared<MRLabel>();
    auto textTransform = textRenderer->getComponent<Transform>();
    textTransform->setPosition(100, 100, 0);// 设置文字位置
    textTransform->setSize(512, 512);// 设置文字区域尺寸

    // 设置文字对齐方式
    textRenderer->setFontColor(0.0f, 0.0f, 0.0f, 1.0f);  // 黑色文字
    textRenderer->setText(L"居中对齐文本", "default");
    textRenderer->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::TOP);

    window->addChild(textRenderer);

    LOG_I("start render");
    engine->render();
    return 0;
}