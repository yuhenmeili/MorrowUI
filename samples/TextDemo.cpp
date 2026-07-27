#include "Engine.h"
#include "elements/MRLabel.h"
#include "base/Transform.h"
#include "FontManager.h"
#include "GlobalObject.h"
#include "ToolUtils.h"
#include "elements/MRButton.h"
#include "elements/MRImage.h"
//
// Created by 0060328 on 25-10-9.
//
using namespace morrow;

int main() {
    EngineOptions engineOptions;
    // engineOptions.multithread = false;
    EngineSharedPtr engine = std::make_shared<Engine>(engineOptions);

    auto window = engine->getWindow();
    window->setClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    FontInfo fontInfo = {
        .name = "debug_morrow_20",
        .path = "assets/fonts/MorrowSansCN1.1-Regular.otf",
        .fontSize = 35.0f
    };
    engine->addFonts({fontInfo});

    const std::string fontName = fontInfo.name;

    auto button1 = MRButton::create();
    button1->setText(L"Button1", fontName);
    auto transform = button1->getComponent<Transform>();
    transform->setPosition(100.0f, 100.0f, 0.0f);
    transform->setSize(100.0f, 100.0f);
    window->addChild(button1);

    const std::wstring text = L"Hello World 测试";
    auto textRenderer = std::make_shared<MRLabel>();
    auto textTransform = textRenderer->getComponent<Transform>();
    textTransform->setPosition(Vector3(880.0f, 220.0f, 0.0f));
    textTransform->setSize(Vector3(700.0f, 240.0f, 0.0f));
    textRenderer->setText(text, fontName);
    textRenderer->setFontColor(1.0f, 0.0f, 0.0f, 1.0f);
    window->addChild(textRenderer);

    int clickCount = 0;
    button1->setOnClickCallback([&]() {
        LOG_I("Button clicked!");
        clickCount++;
        textRenderer->setText(L"Clicked " + std::to_wstring(clickCount) + L"!", fontName);
    });

    engine->render();
    return 0;
}