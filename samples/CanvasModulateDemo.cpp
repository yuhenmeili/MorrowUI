#include "Engine.h"
#include "FontManager.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "elements/MRCanvasModulate.h"
#include "elements/MRColor.h"
#include "elements/MRLabel.h"

using namespace morrow;

namespace {

std::shared_ptr<MRLabel> createLabel(const std::wstring& text, float x, float y, float width, float fontSize, const Vector4& color) {
    auto label = std::make_shared<MRLabel>();
    label->setText(text, "default");
    label->setFontSize(fontSize);
    label->setFontColor(color);
    label->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    label->getComponent<Transform>()->setPosition(x, y, 0.0f);
    label->getComponent<Transform>()->setSize(width, 54.0f);
    return label;
}

}  // namespace

int main() {
    auto engine = std::make_shared<Engine>();
    auto window = engine->getWindow();
    window->setClearColor(0.92f, 0.95f, 0.99f, 1.0f);

    FontInfo fontInfo = {.name = "default", .path = "assets/fonts/MorrowSansCN1.1-Regular.otf"};
    engine->addFonts({fontInfo});

    auto header = MRColor::create();
    header->setColor(0.12f, 0.48f, 0.78f, 1.0f);
    header->getComponent<Transform>()->setPosition(0.0f, 0.0f, 0.0f);
    header->getComponent<Transform>()->setSize(1920.0f, 180.0f);
    window->addChild(header);

    window->addChild(createLabel(L"MRCanvasModulate 全局色调演示", 120.0f, 55.0f, 900.0f, 34.0f, Vector4(1.0f, 1.0f, 1.0f, 1.0f)));

    auto warmCard = MRColor::create();
    warmCard->setColor(1.0f, 0.62f, 0.24f, 1.0f);
    warmCard->setRounding(18.0f);
    warmCard->getComponent<Transform>()->setPosition(180.0f, 280.0f, 0.0f);
    warmCard->getComponent<Transform>()->setSize(460.0f, 300.0f);
    window->addChild(warmCard);
    window->addChild(createLabel(L"暖色卡片", 230.0f, 390.0f, 300.0f, 32.0f, Vector4(0.18f, 0.1f, 0.04f, 1.0f)));

    auto greenCard = MRColor::create();
    greenCard->setColor(0.22f, 0.78f, 0.48f, 1.0f);
    greenCard->setRounding(18.0f);
    greenCard->getComponent<Transform>()->setPosition(730.0f, 280.0f, 0.0f);
    greenCard->getComponent<Transform>()->setSize(460.0f, 300.0f);
    window->addChild(greenCard);
    window->addChild(createLabel(L"状态卡片", 780.0f, 390.0f, 300.0f, 32.0f, Vector4(0.03f, 0.16f, 0.08f, 1.0f)));

    auto violetCard = MRColor::create();
    violetCard->setColor(0.62f, 0.4f, 0.9f, 1.0f);
    violetCard->setRounding(18.0f);
    violetCard->getComponent<Transform>()->setPosition(1280.0f, 280.0f, 0.0f);
    violetCard->getComponent<Transform>()->setSize(460.0f, 300.0f);
    window->addChild(violetCard);
    window->addChild(createLabel(L"氛围卡片", 1330.0f, 390.0f, 300.0f, 32.0f, Vector4(0.12f, 0.06f, 0.22f, 1.0f)));

    auto modulate = MRCanvasModulate::create();
    modulate->setModulateColor(Vector4(0.38f, 0.48f, 0.68f, 1.0f));
    modulate->setStrength(0.0f);
    window->addChild(modulate);

    auto toggleButton = MRButton::create();
    toggleButton->setText(L"切换夜间模式", "default");
    toggleButton->setTextFontSize(26.0f);
    toggleButton->setTextColor(0.08f, 0.12f, 0.18f, 1.0f);
    toggleButton->setBackgroundColor(0.82f, 0.88f, 0.94f, 1.0f);
    toggleButton->setHoverColor(Vector4(0.72f, 0.82f, 0.91f, 1.0f));
    toggleButton->setPressedColor(Vector4(0.62f, 0.75f, 0.87f, 1.0f));
    toggleButton->setCornerRadius(10.0f);
    toggleButton->getComponent<Transform>()->setPosition(760.0f, 700.0f, 0.0f);
    toggleButton->getComponent<Transform>()->setSize(400.0f, 72.0f);
    toggleButton->setOnClickCallback([modulate]() {
        modulate->setNightMode(!modulate->isNightMode());
        LOG_I("night mode = {}", modulate->isNightMode());
    });
    window->addChild(toggleButton);

    window->addChild(createLabel(L"乘法调制会同时影响背景、色块、文字和按钮", 610.0f, 810.0f, 800.0f, 24.0f, Vector4(0.25f, 0.3f, 0.38f, 1.0f)));

    LOG_I("CanvasModulate demo started");
    engine->render();
    return 0;
}
