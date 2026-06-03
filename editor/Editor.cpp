//
// Created by lance on 2025/12/15.
//
#include <memory>

#include "Engine.h"
#include "FontManager.h"
#include "base/Transform.h"
#include "elements/MRButton.h"

using namespace morrow;
int main() {
    EngineOptions engineOptions;
    engineOptions.multithread = false;
    engineOptions.enableRequestRender = false;
    EngineSharedPtr engine = std::make_shared<morrow::Engine>(engineOptions);

    auto window = engine->getWindow();
    window->setClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    FontInfo fontInfo = {
        .name = "default",
        .path = "assets/fonts/MorrowSansCN1.1-Regular.otf",
        .fontSize = 32.0f
    };
    engine->addFonts({fontInfo});

    auto button = MRButton::create();
    button->setText(L"Button", "default");

    auto transform = button->getComponent<Transform>();
    transform->setPosition(100.0f, 100.0f, 0.0f);
    transform->setSize(100.0f, 100.0f);

    window->addChild(button);
    engine->render();
    return 0;
}