#include "Engine.h"
#include "FontManager.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "elements/MRColor.h"
#include "elements/MRLabel.h"
#include "elements/MRSeparator.h"
#include "elements/MRSpacer.h"
#include "elements/MRSpinBox.h"
#include "layout/HBoxContainer.h"

using namespace morrow;

namespace {

std::shared_ptr<MRLabel> createLabel(const std::wstring& text, float x, float y, float width, float fontSize = 22.0f) {
    auto label = std::make_shared<MRLabel>();
    label->setText(text, "default");
    label->setFontSize(fontSize);
    label->setFontColor(0.12f, 0.15f, 0.2f, 1.0f);
    label->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    label->getComponent<Transform>()->setPosition(x, y, 0.0f);
    label->getComponent<Transform>()->setSize(width, 42.0f);
    return label;
}

std::shared_ptr<MRButton> createActionButton(const std::wstring& text) {
    auto button = MRButton::create();
    button->setText(text, "default");
    button->setTextFontSize(20.0f);
    button->setTextColor(0.08f, 0.12f, 0.17f, 1.0f);
    button->setBackgroundColor(0.82f, 0.88f, 0.94f, 1.0f);
    button->setHoverColor(Vector4(0.72f, 0.82f, 0.91f, 1.0f));
    button->setPressedColor(Vector4(0.62f, 0.75f, 0.87f, 1.0f));
    button->setCornerRadius(6.0f);
    button->getComponent<Transform>()->setSize(132.0f, 48.0f);
    return button;
}

}  // namespace

int main() {
    auto engine = std::make_shared<Engine>();
    auto window = engine->getWindow();
    window->setClearColor(0.96f, 0.97f, 0.99f, 1.0f);

    FontInfo fontInfo = {.name = "default", .path = "assets/fonts/MorrowSansCN1.1-Regular.otf"};
    engine->addFonts({fontInfo});

    constexpr float panelX = 160.0f;
    constexpr float panelY = 100.0f;
    constexpr float panelWidth = 820.0f;

    auto panel = MRColor::create();
    panel->setColor(1.0f, 1.0f, 1.0f, 1.0f);
    panel->setRounding(8.0f);
    panel->getComponent<Transform>()->setPosition(panelX, panelY, 0.0f);
    panel->getComponent<Transform>()->setSize(panelWidth, 520.0f);
    window->addChild(panel);

    window->addChild(createLabel(L"SpinBox / Separator / Spacer", panelX + 36.0f, panelY + 24.0f, 620.0f, 30.0f));

    auto topSeparator = MRHSeparator::create();
    topSeparator->getComponent<Transform>()->setPosition(panelX + 36.0f, panelY + 78.0f, 0.0f);
    topSeparator->getComponent<Transform>()->setSize(panelWidth - 72.0f, 2.0f);
    window->addChild(topSeparator);

    window->addChild(createLabel(L"座舱温度", panelX + 36.0f, panelY + 112.0f, 180.0f));
    auto temperature = MRSpinBox::create();
    temperature->getComponent<Transform>()->setPosition(panelX + 230.0f, panelY + 106.0f, 0.0f);
    temperature->getComponent<Transform>()->setSize(260.0f, 56.0f);
    temperature->setRange(16.0, 30.0);
    temperature->setStep(0.5);
    temperature->setDecimals(1);
    temperature->setSuffix(L" C");
    temperature->setValue(22.5);
    temperature->setOnValueChangedCallback([](double value) { LOG_I("temperature = {}", value); });
    window->addChild(temperature);

    auto verticalSeparator = MRVSeparator::create();
    verticalSeparator->getComponent<Transform>()->setPosition(panelX + 530.0f, panelY + 100.0f, 0.0f);
    verticalSeparator->getComponent<Transform>()->setSize(2.0f, 150.0f);
    window->addChild(verticalSeparator);

    window->addChild(createLabel(L"预约时间", panelX + 570.0f, panelY + 112.0f, 180.0f));
    auto hour = MRSpinBox::create();
    hour->getComponent<Transform>()->setPosition(panelX + 570.0f, panelY + 158.0f, 0.0f);
    hour->getComponent<Transform>()->setSize(180.0f, 56.0f);
    hour->setRange(0.0, 23.0);
    hour->setStep(1.0);
    hour->setDecimals(0);
    hour->setSuffix(L" h");
    hour->setValue(8.0);
    window->addChild(hour);

    auto middleSeparator = MRHSeparator::create();
    middleSeparator->getComponent<Transform>()->setPosition(panelX + 36.0f, panelY + 282.0f, 0.0f);
    middleSeparator->getComponent<Transform>()->setSize(panelWidth - 72.0f, 2.0f);
    window->addChild(middleSeparator);

    window->addChild(createLabel(L"Spacer 弹性占位：按钮自动分布到容器两端", panelX + 36.0f, panelY + 304.0f, 620.0f, 20.0f));

    auto actionRow = std::make_shared<HBoxContainer>();
    actionRow->setSpacing(12.0f);
    actionRow->getComponent<Transform>()->setPosition(panelX + 36.0f, panelY + 360.0f, 0.0f);
    actionRow->getComponent<Transform>()->setSize(panelWidth - 72.0f, 48.0f);

    auto cancel = createActionButton(L"取消");
    auto spacer = MRSpacer::create(1.0f);
    spacer->setMinimumSize(Vector2(24.0f, 1.0f));
    auto apply = createActionButton(L"应用设置");
    cancel->setOnClickCallback([]() { LOG_I("cancel settings"); });
    apply->setOnClickCallback([]() { LOG_I("apply settings"); });
    actionRow->addChild(cancel);
    actionRow->addChild(spacer);
    actionRow->addChild(apply);
    window->addChild(actionRow);

    auto bottomSeparator = MRHSeparator::create();
    bottomSeparator->setColor(0.24f, 0.52f, 0.7f, 1.0f);
    bottomSeparator->getComponent<Transform>()->setPosition(panelX + 36.0f, panelY + 446.0f, 0.0f);
    bottomSeparator->getComponent<Transform>()->setSize(panelWidth - 72.0f, 3.0f);
    window->addChild(bottomSeparator);

    LOG_I("SpinBox, separator and spacer demo started");
    engine->render();
    return 0;
}
