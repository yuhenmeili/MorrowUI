#include "Engine.h"
#include "FontManager.h"
#include "Texture.h"
#include "base/Transform.h"
#include "elements/MRGearsIris.h"
#include "elements/MRGearsOpening.h"
#include "elements/MRGearsSelect.h"
#include "elements/MRGearsShine.h"
#include "elements/MRLabel.h"

using namespace morrow;

namespace {

std::shared_ptr<MRLabel> createLabel(
    const std::wstring& text,
    float x,
    float y,
    float width,
    float height,
    float fontSize = 22.0f) {
    auto label = std::make_shared<MRLabel>();
    label->setText(text, "default");
    label->setFontSize(fontSize);
    label->setFontColor(0.88f, 0.91f, 0.97f, 1.0f);
    label->setAlign(HorizontalAlignment::CENTER, VerticalAlignment::CENTER);
    label->getComponent<Transform>()->setPosition(x, y, 0.0f);
    label->getComponent<Transform>()->setSize(width, height);
    return label;
}

}  // namespace

int main() {
    auto engine = std::make_shared<Engine>();
    auto window = engine->getWindow();
    window->setClearColor(0.02f, 0.025f, 0.04f, 1.0f);
    engine->addFonts({FontInfo{
        .name = "default",
        .path = "assets/fonts/MorrowSansCN1.1-Regular.otf",
    }});

    window->addChild(createLabel(L"Gears Effects Showcase", 100.0f, 24.0f, 1720.0f, 54.0f, 34.0f));
    window->addChild(createLabel(
        L"MRGearsIris / MRGearsOpening / MRGearsShine / MRGearsSelect",
        100.0f, 74.0f, 1720.0f, 40.0f, 20.0f));

    window->addChild(createLabel(L"MRGearsIris", 110.0f, 125.0f, 300.0f, 42.0f));
    auto iris = MRGearsIris::create();
    iris->getComponent<Transform>()->setPosition(188.0f, 175.0f, 0.0f);
    iris->getComponent<Transform>()->setSize(144.0f, 760.0f);
    iris->initialize();
    window->addChild(iris);

    window->addChild(createLabel(L"MRGearsOpening", 500.0f, 125.0f, 340.0f, 42.0f));
    auto opening = MRGearsOpening::create();
    opening->getComponent<Transform>()->setPosition(571.0f, 195.0f, 0.0f);
    opening->getComponent<Transform>()->setSize(198.0f, 708.0f);

    auto openingTexture = Texture::create(ImageType::IMAGE);
    openingTexture->setImageUrl("assets/textures/gearBG.png");
    opening->getComponent<MeshRenderer>()->getMaterial()->setTexture("texture", openingTexture);
    opening->initialize();
    window->addChild(opening);

    window->addChild(createLabel(L"MRGearsShine", 900.0f, 125.0f, 340.0f, 42.0f));
    auto shine = MRGearsShine::create();
    shine->getComponent<Transform>()->setPosition(998.0f, 205.0f, 0.0f);
    shine->getComponent<Transform>()->setSize(144.0f, 684.0f);
    shine->initialize();
    window->addChild(shine);

    window->addChild(createLabel(L"MRGearsSelect", 1320.0f, 125.0f, 360.0f, 42.0f));
    window->addChild(createLabel(
        L"呼吸亮度选中点",
        1320.0f, 185.0f, 360.0f, 42.0f, 19.0f));
    auto select = MRGearsSelect::create();
    select->getComponent<Transform>()->setPosition(1495.0f, 430.0f, 0.0f);
    select->getComponent<Transform>()->setSize(10.0f, 10.0f);
    select->initialize();
    window->addChild(select);

    LOG_I("GearsDemo started");
    engine->render();
    return 0;
}
