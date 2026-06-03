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

    button1->setOnClickCallback([button1]() {
        LOG_I("Button clicked!");
        button1->setText(L"Clicked!", "debug_morrow_20");
    });

    const std::wstring text = L"Hello World 测试";

    auto font = GlobalObject::getInstance().getFontManager()->getFont(fontName);
    if (font) {
        font->EnsureStringGlyphs(text);

        auto atlasImage = MRImage::create();
        auto atlasTransform = atlasImage->getComponent<Transform>();
        atlasTransform->setPosition(320.0f, 320.0f, 0.0f);
        atlasTransform->setSize(512.0f, 512.0f);
        atlasImage->setTexture(font->GetTextureAtlas());
        window->addChild(atlasImage);

        LOG_I("Font '{}' atlas texture size: {}x{}", fontName, font->GetTextureAtlas()->getWidth(), font->GetTextureAtlas()->getHeight());
    }

    auto textRenderer = std::make_shared<MRLabel>();
    auto textTransform = textRenderer->getComponent<Transform>();
    textTransform->setPosition(Vector3(880.0f, 220.0f, 0.0f));
    textTransform->setSize(Vector3(700.0f, 240.0f, 0.0f));
    textRenderer->setText(text, fontName);
    textRenderer->setFontColor(1.0f, 0.0f, 0.0f, 1.0f);

    window->addChild(textRenderer);

    engine->afterRender().add([&]() {
        textRenderer->setText(L"Hello World 测试", fontName);
    });

    engine->render();
    return 0;
}
