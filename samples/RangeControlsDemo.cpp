#include <cwchar>

#include "Engine.h"
#include "FontManager.h"
#include "base/Transform.h"
#include "elements/MRLabel.h"
#include "elements/MRProgressBar.h"
#include "elements/MRSlider.h"
#include "ui/helpers/Tween.h"

using namespace morrow;

namespace {

std::shared_ptr<MRLabel> createLabel(const std::wstring& text, float x, float y, float width, float height, float fontSize = 22.0f) {
    auto label = std::make_shared<MRLabel>();
    label->setText(text, "default");
    label->setFontSize(fontSize);
    label->setFontColor(0.12f, 0.16f, 0.22f, 1.0f);
    label->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    label->getComponent<Transform>()->setPosition(x, y, 0.0f);
    label->getComponent<Transform>()->setSize(width, height);
    return label;
}

}  // namespace

int main() {
    auto engine = std::make_shared<Engine>();
    auto window = engine->getWindow();
    window->setClearColor(0.97f, 0.98f, 1.0f, 1.0f);
    engine->addFonts({FontInfo{
        .name = "default",
        .path = "assets/fonts/MorrowSansCN1.1-Regular.otf",
    }});

    window->addChild(createLabel(L"Range Controls：Slider / ProgressBar", 100.0f, 42.0f, 1200.0f, 54.0f, 34.0f));
    window->addChild(createLabel(L"拖动 Slider 可联动 ProgressBar；下方进度条展示 Tween、反向和纵向填充。", 100.0f, 94.0f, 1500.0f, 40.0f, 20.0f));

    window->addChild(createLabel(L"水平 Slider 联动进度", 140.0f, 165.0f, 600.0f, 40.0f));
    auto linkedBar = MRProgressBar::create();
    linkedBar->getComponent<Transform>()->setPosition(140.0f, 285.0f, 0.0f);
    linkedBar->getComponent<Transform>()->setSize(600.0f, 28.0f);
    linkedBar->setTrackColor(0.85f, 0.88f, 0.92f, 1.0f);
    linkedBar->setFillColor(0.2f, 0.6f, 0.9f, 1.0f);
    linkedBar->setRounding(14.0f);
    linkedBar->setProgress(0.3f);
    window->addChild(linkedBar);

    auto valueLabel = createLabel(L"当前值：30%", 140.0f, 325.0f, 600.0f, 40.0f, 20.0f);
    window->addChild(valueLabel);

    auto horizontalSlider = MRSlider::create();
    horizontalSlider->getComponent<Transform>()->setPosition(140.0f, 225.0f, 0.0f);
    horizontalSlider->getComponent<Transform>()->setSize(600.0f, 24.0f);
    horizontalSlider->setTrackColor(0.85f, 0.88f, 0.92f, 1.0f);
    horizontalSlider->setFillColor(0.2f, 0.6f, 0.9f, 1.0f);
    horizontalSlider->setRounding(12.0f);
    horizontalSlider->setThumbSize(Vector2(34.0f, 34.0f));
    horizontalSlider->setThumbColor(0.1f, 0.4f, 0.8f, 1.0f);
    horizontalSlider->setThumbRounding(17.0f);
    horizontalSlider->setValue(0.3f);
    horizontalSlider->setOnValueChangedCallback([linkedBar, valueLabel](float value) {
        linkedBar->setProgress(value);
        wchar_t text[64];
        std::swprintf(text, 64, L"当前值：%d%%", static_cast<int>(value * 100.0f + 0.5f));
        valueLabel->setText(text, "default");
        LOG_I("horizontal slider value = {}", value);
    });
    window->addChild(horizontalSlider);

    window->addChild(createLabel(L"纵向 Slider / ProgressBar", 930.0f, 165.0f, 500.0f, 40.0f));
    auto verticalSlider = MRSlider::create();
    verticalSlider->getComponent<Transform>()->setPosition(980.0f, 225.0f, 0.0f);
    verticalSlider->getComponent<Transform>()->setSize(28.0f, 330.0f);
    verticalSlider->setDirection(ProgressDirection::BottomToTop);
    verticalSlider->setTrackColor(0.85f, 0.88f, 0.92f, 1.0f);
    verticalSlider->setFillColor(0.3f, 0.8f, 0.4f, 1.0f);
    verticalSlider->setRounding(14.0f);
    verticalSlider->setThumbSize(Vector2(38.0f, 38.0f));
    verticalSlider->setThumbColor(0.1f, 0.6f, 0.3f, 1.0f);
    verticalSlider->setThumbRounding(19.0f);
    verticalSlider->setValue(0.6f);
    window->addChild(verticalSlider);

    auto verticalBar = MRProgressBar::create();
    verticalBar->getComponent<Transform>()->setPosition(1110.0f, 225.0f, 0.0f);
    verticalBar->getComponent<Transform>()->setSize(28.0f, 330.0f);
    verticalBar->setDirection(ProgressDirection::BottomToTop);
    verticalBar->setTrackColor(0.85f, 0.88f, 0.92f, 1.0f);
    verticalBar->setFillColor(0.3f, 0.8f, 0.4f, 1.0f);
    verticalBar->setRounding(14.0f);
    verticalBar->setProgress(0.6f);
    window->addChild(verticalBar);

    verticalSlider->setOnValueChangedCallback([verticalBar](float value) {
        verticalBar->setProgress(value);
        LOG_I("vertical slider value = {}", value);
    });

    window->addChild(createLabel(L"自动循环进度（Tween）", 140.0f, 470.0f, 600.0f, 40.0f));
    auto animatedBar = MRProgressBar::create();
    animatedBar->getComponent<Transform>()->setPosition(140.0f, 530.0f, 0.0f);
    animatedBar->getComponent<Transform>()->setSize(600.0f, 28.0f);
    animatedBar->setTrackColor(0.85f, 0.88f, 0.92f, 1.0f);
    animatedBar->setFillColor(0.55f, 0.35f, 0.9f, 1.0f);
    animatedBar->setRounding(14.0f);
    window->addChild(animatedBar);

    auto tween = Tween::create(0.0f, 1.0f, 2.0f);
    tween->setEase(EaseType::Linear).onUpdate([animatedBar](float value) {
        animatedBar->setProgress(value);
    }).onComplete([tween]() { tween->restart(); });
    tween->play();
    TweenManager::getInstance().addTween(tween);

    window->addChild(createLabel(L"右到左固定进度 70%", 140.0f, 620.0f, 600.0f, 40.0f));
    auto reverseBar = MRProgressBar::create();
    reverseBar->getComponent<Transform>()->setPosition(140.0f, 680.0f, 0.0f);
    reverseBar->getComponent<Transform>()->setSize(600.0f, 28.0f);
    reverseBar->setDirection(ProgressDirection::RightToLeft);
    reverseBar->setTrackColor(0.9f, 0.9f, 0.9f, 1.0f);
    reverseBar->setFillColor(0.9f, 0.4f, 0.2f, 1.0f);
    reverseBar->setRounding(14.0f);
    reverseBar->setProgress(0.7f);
    window->addChild(reverseBar);

    LOG_I("RangeControlsDemo started");
    engine->render();
    return 0;
}
