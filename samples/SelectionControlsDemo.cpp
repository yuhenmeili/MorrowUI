#include "Engine.h"
#include "FontManager.h"
#include "base/Transform.h"
#include "elements/MRCheckBox.h"
#include "elements/MRCheckButton.h"
#include "elements/MRLabel.h"
#include "elements/MRRadioButton.h"
#include "elements/MRToggle.h"

using namespace morrow;

namespace {

template <typename T>
void configureButton(const std::shared_ptr<T>& button, const std::wstring& text, float x, float y, float width = 286.0f) {
    button->setText(text, "default");
    button->setTextFontSize(20.0f);
    button->setTextColor(0.1f, 0.12f, 0.16f, 1.0f);
    button->setBackgroundColor(0.9f, 0.92f, 0.95f, 1.0f);
    button->setHoverColor(Vector4(0.82f, 0.88f, 0.94f, 1.0f));
    button->setPressedColor(Vector4(0.72f, 0.82f, 0.92f, 1.0f));
    button->setCheckedColor(0.2f, 0.65f, 0.5f, 1.0f);
    button->setCheckedHoverColor(Vector4(0.25f, 0.72f, 0.57f, 1.0f));
    button->setCheckedPressedColor(Vector4(0.15f, 0.52f, 0.4f, 1.0f));
    button->setCornerRadius(8.0f);
    button->template getComponent<Transform>()->setPosition(x, y, 0.0f);
    button->template getComponent<Transform>()->setSize(width, 48.0f);
}

std::shared_ptr<MRLabel> createSectionTitle(const std::wstring& text, float x) {
    auto title = std::make_shared<MRLabel>();
    title->setText(text, "default");
    title->setFontSize(18.0f);
    title->setFontColor(0.3f, 0.34f, 0.4f, 1.0f);
    title->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    title->getComponent<Transform>()->setPosition(x, 24.0f, 0.0f);
    title->getComponent<Transform>()->setSize(286.0f, 28.0f);
    return title;
}

}  // namespace

int main() {
    EngineOptions options;
    // options.windowInfo.width = 700;
    // options.windowInfo.height = 280;
    auto engine = std::make_shared<Engine>(options);
    auto window = engine->getWindow();
    window->setClearColor(0.97f, 0.98f, 1.0f, 1.0f);

    FontInfo fontInfo = {.name = "default", .path = "assets/fonts/MorrowSansCN1.1-Regular.otf"};
    engine->addFonts({fontInfo});

    constexpr float leftColumnX = 28.0f;
    constexpr float rightColumnX = 364.0f;
    constexpr float firstRowY = 64.0f;
    constexpr float secondRowY = 128.0f;
    constexpr float thirdRowY = 192.0f;

    window->addChild(createSectionTitle(L"多选与开关", leftColumnX));
    window->addChild(createSectionTitle(L"驾驶模式（单选）", rightColumnX));

    auto checkBox = MRCheckBox::create();
    configureButton(checkBox, L"启用空调", leftColumnX, firstRowY);
    checkBox->setOnCheckedChangedCallback([](bool checked) { LOG_I("CheckBox checked = {}", checked); });
    window->addChild(checkBox);

    auto checkButton = MRCheckButton::create();
    configureButton(checkButton, L"座椅加热", leftColumnX, secondRowY);
    checkButton->setChecked(true);
    checkButton->setOnCheckedChangedCallback([](bool checked) { LOG_I("CheckButton checked = {}", checked); });
    window->addChild(checkButton);

    auto toggle = MRToggle::create();
    configureButton(toggle, L"自动大灯", leftColumnX, thirdRowY);
    toggle->setOnCheckedChangedCallback([](bool checked) { LOG_I("Toggle value = {}", checked); });
    window->addChild(toggle);

    auto group = std::make_shared<MRRadioGroup>();
    auto radioA = MRRadioButton::create();
    configureButton(radioA, L"舒适模式", rightColumnX, firstRowY);
    radioA->setGroup(group);
    radioA->setOnCheckedChangedCallback([](bool checked) {
        if (checked)
            LOG_I("Radio selection = comfort");
    });
    window->addChild(radioA);

    auto radioB = MRRadioButton::create();
    configureButton(radioB, L"运动模式", rightColumnX, secondRowY);
    radioB->setGroup(group);
    radioB->setOnCheckedChangedCallback([](bool checked) {
        if (checked)
            LOG_I("Radio selection = sport");
    });
    window->addChild(radioB);

    auto radioC = MRRadioButton::create();
    configureButton(radioC, L"节能模式", rightColumnX, thirdRowY);
    radioC->setGroup(group);
    radioC->setOnCheckedChangedCallback([](bool checked) {
        if (checked)
            LOG_I("Radio selection = eco");
    });
    window->addChild(radioC);
    group->select(radioA);

    LOG_I("Selection controls demo: checkboxes allow multiple selections, radio buttons are exclusive, toggle switches state");
    engine->render();
    return 0;
}
