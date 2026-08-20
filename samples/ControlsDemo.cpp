#include "Engine.h"
#include "FontManager.h"
#include "Texture.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "elements/MRCheckBox.h"
#include "elements/MRCheckButton.h"
#include "elements/MRColor.h"
#include "elements/MRLabel.h"
#include "elements/MRMenuButton.h"
#include "elements/MROptionButton.h"
#include "elements/MRPopupMenu.h"
#include "elements/MRRadioButton.h"
#include "elements/MRSeparator.h"
#include "elements/MRSpacer.h"
#include "elements/MRSpinBox.h"
#include "elements/MRTextureButton.h"
#include "elements/MRToggle.h"
#include "layout/HBoxContainer.h"

using namespace morrow;

namespace {

constexpr float kPanelTop = 140.0f;

std::shared_ptr<MRLabel> createLabel(const std::wstring& text, float x, float y, float width, float height, float fontSize = 20.0f,
                                     HorizontalAlignment horizontal = HorizontalAlignment::LEFT) {
    auto label = std::make_shared<MRLabel>();
    label->setText(text, "default");
    label->setFontSize(fontSize);
    label->setFontColor(0.12f, 0.16f, 0.22f, 1.0f);
    label->setAlign(horizontal, VerticalAlignment::CENTER);
    label->getComponent<Transform>()->setPosition(x, y, 0.0f);
    label->getComponent<Transform>()->setSize(width, height);
    return label;
}

std::shared_ptr<MRColor> createPanel(float x, float y, float width, float height) {
    auto panel = MRColor::create();
    panel->setColor(0.99f, 0.995f, 1.0f, 1.0f);
    panel->setRounding(12.0f);
    panel->getComponent<Transform>()->setPosition(x, y, -0.1f);
    panel->getComponent<Transform>()->setSize(width, height);
    return panel;
}

void configureButton(const std::shared_ptr<MRButton>& button, const std::wstring& text, float x, float y, float width, float height = 48.0f) {
    button->setText(text, "default");
    button->setTextFontSize(19.0f);
    button->setTextColor(0.1f, 0.12f, 0.16f, 1.0f);
    button->setBackgroundColor(0.9f, 0.92f, 0.95f, 1.0f);
    button->setHoverColor(Vector4(0.82f, 0.88f, 0.94f, 1.0f));
    button->setPressedColor(Vector4(0.72f, 0.82f, 0.92f, 1.0f));
    button->setCornerRadius(8.0f);
    button->getComponent<Transform>()->setPosition(x, y, 0.0f);
    button->getComponent<Transform>()->setSize(width, height);
}

template <typename T>
void configureSelectionControl(const std::shared_ptr<T>& button, const std::wstring& text, float x, float y, float width = 215.0f) {
    button->setText(text, "default");
    button->setTextFontSize(18.0f);
    button->setTextColor(0.1f, 0.12f, 0.16f, 1.0f);
    button->setBackgroundColor(0.9f, 0.92f, 0.95f, 1.0f);
    button->setHoverColor(Vector4(0.82f, 0.88f, 0.94f, 1.0f));
    button->setPressedColor(Vector4(0.72f, 0.82f, 0.92f, 1.0f));
    button->setCheckedColor(0.2f, 0.65f, 0.5f, 1.0f);
    button->setCheckedHoverColor(Vector4(0.25f, 0.72f, 0.57f, 1.0f));
    button->setCheckedPressedColor(Vector4(0.15f, 0.52f, 0.4f, 1.0f));
    button->setCornerRadius(8.0f);
    button->template getComponent<Transform>()->setPosition(x, y, 0.0f);
    button->template getComponent<Transform>()->setSize(width, 46.0f);
}

void configureMenuButton(const std::shared_ptr<MRButton>& button, const std::wstring& text, float x, float y, float width) {
    configureButton(button, text, x, y, width);
    button->setTextFontSize(18.0f);
}

std::shared_ptr<MRButton> createActionButton(const std::wstring& text) {
    auto button = MRButton::create();
    configureButton(button, text, 0.0f, 0.0f, 132.0f);
    return button;
}

}  // namespace

int main() {
    auto engine = std::make_shared<Engine>();
    auto window = engine->getWindow();
    window->setClearColor(0.94f, 0.96f, 0.99f, 1.0f);
    engine->addFonts({FontInfo{
        .name = "default",
        .path = "assets/fonts/MorrowSansCN1.1-Regular.otf",
    }});

    window->addChild(createLabel(L"Controls Showcase：按钮、选择、菜单、数值输入与布局", 60.0f, 30.0f, 1700.0f, 54.0f, 34.0f));
    window->addChild(createLabel(L"统一展示常用交互控件及其回调、状态和容器组合方式。", 60.0f, 84.0f, 1700.0f, 36.0f, 20.0f));

    // ---------------------------------------------------------------------
    // Button / TextureButton / HBoxContainer
    // ---------------------------------------------------------------------
    constexpr float buttonPanelX = 60.0f;
    constexpr float buttonPanelW = 420.0f;
    window->addChild(createPanel(buttonPanelX, kPanelTop, buttonPanelW, 800.0f));
    window->addChild(createLabel(L"Button / TextureButton", buttonPanelX + 24.0f, kPanelTop + 22.0f, buttonPanelW - 48.0f, 38.0f, 23.0f));

    auto buttonRow = std::make_shared<HBoxContainer>();
    buttonRow->setSpacing(12.0f);
    buttonRow->getComponent<Transform>()->setPosition(buttonPanelX + 24.0f, kPanelTop + 88.0f, 0.0f);
    buttonRow->getComponent<Transform>()->setSize(buttonPanelW - 48.0f, 58.0f);

    auto textButton = MRButton::create();
    configureButton(textButton, L"普通按钮", 0.0f, 0.0f, 180.0f);
    textButton->setOnClickCallback([]() { LOG_I("normal button clicked"); });
    buttonRow->addChild(textButton);

    auto colorButton = MRButton::create();
    configureButton(colorButton, L"彩色按钮", 0.0f, 0.0f, 180.0f);
    colorButton->setBackgroundColor(0.9f, 0.35f, 0.28f, 1.0f);
    colorButton->setHoverColor(Vector4(0.96f, 0.48f, 0.38f, 1.0f));
    colorButton->setTextColor(1.0f, 1.0f, 1.0f, 1.0f);
    buttonRow->addChild(colorButton);
    window->addChild(buttonRow);

    auto textureButton = MRTextureButton::create();
    auto normalTexture = Texture::create();
    normalTexture->setImageUrl("assets/textures/tailgate_off.png");
    auto hoverTexture = Texture::create();
    hoverTexture->setImageUrl("assets/textures/tailgate_on.png");
    auto disabledTexture = Texture::create();
    disabledTexture->setImageUrl("assets/textures/tailgate_disable.png");
    textureButton->setNormalTexture(normalTexture);
    textureButton->setHoverTexture(hoverTexture);
    textureButton->setPressedTexture(hoverTexture);
    textureButton->setFocusedTexture(hoverTexture);
    textureButton->setDisabledTexture(disabledTexture);
    textureButton->setOnClickCallback([]() { LOG_I("texture button clicked"); });
    textureButton->getComponent<Transform>()->setPosition(buttonPanelX + 110.0f, kPanelTop + 190.0f, 0.0f);
    textureButton->getComponent<Transform>()->setSize(200.0f, 200.0f);
    window->addChild(textureButton);
    window->addChild(createLabel(L"MRTextureButton\nNormal / Hover / Pressed / Disabled", buttonPanelX + 30.0f, kPanelTop + 405.0f, buttonPanelW - 60.0f, 80.0f, 18.0f,
                                 HorizontalAlignment::CENTER));

    // ---------------------------------------------------------------------
    // CheckBox / CheckButton / Toggle / RadioButton
    // ---------------------------------------------------------------------
    constexpr float selectionPanelX = 520.0f;
    constexpr float selectionPanelW = 540.0f;
    window->addChild(createPanel(selectionPanelX, kPanelTop, selectionPanelW, 800.0f));
    window->addChild(createLabel(L"Selection Controls", selectionPanelX + 24.0f, kPanelTop + 22.0f, selectionPanelW - 48.0f, 38.0f, 23.0f));
    window->addChild(createLabel(L"多选与开关", selectionPanelX + 24.0f, kPanelTop + 82.0f, 230.0f, 32.0f, 18.0f));
    window->addChild(createLabel(L"RadioGroup 单选", selectionPanelX + 278.0f, kPanelTop + 82.0f, 230.0f, 32.0f, 18.0f));

    const float selectionLeft = selectionPanelX + 24.0f;
    const float selectionRight = selectionPanelX + 278.0f;
    const float selectionY = kPanelTop + 130.0f;

    auto checkBox = MRCheckBox::create();
    configureSelectionControl(checkBox, L"启用空调", selectionLeft, selectionY);
    checkBox->setOnCheckedChangedCallback([](bool checked) { LOG_I("CheckBox checked = {}", checked); });
    window->addChild(checkBox);

    auto checkButton = MRCheckButton::create();
    configureSelectionControl(checkButton, L"座椅加热", selectionLeft, selectionY + 62.0f);
    checkButton->setChecked(true);
    checkButton->setOnCheckedChangedCallback([](bool checked) { LOG_I("CheckButton checked = {}", checked); });
    window->addChild(checkButton);

    auto toggle = MRToggle::create();
    configureSelectionControl(toggle, L"自动大灯", selectionLeft, selectionY + 124.0f);
    toggle->setOnCheckedChangedCallback([](bool checked) { LOG_I("Toggle value = {}", checked); });
    window->addChild(toggle);

    auto group = std::make_shared<MRRadioGroup>();
    auto radioA = MRRadioButton::create();
    configureSelectionControl(radioA, L"舒适模式", selectionRight, selectionY);
    radioA->setGroup(group);
    radioA->setOnCheckedChangedCallback([](bool checked) {
        if (checked)
            LOG_I("Radio selection = comfort");
    });
    window->addChild(radioA);

    auto radioB = MRRadioButton::create();
    configureSelectionControl(radioB, L"运动模式", selectionRight, selectionY + 62.0f);
    radioB->setGroup(group);
    radioB->setOnCheckedChangedCallback([](bool checked) {
        if (checked)
            LOG_I("Radio selection = sport");
    });
    window->addChild(radioB);

    auto radioC = MRRadioButton::create();
    configureSelectionControl(radioC, L"节能模式", selectionRight, selectionY + 124.0f);
    radioC->setGroup(group);
    radioC->setOnCheckedChangedCallback([](bool checked) {
        if (checked)
            LOG_I("Radio selection = eco");
    });
    window->addChild(radioC);
    group->select(radioA);

    window->addChild(
        createLabel(L"CheckBox 支持多选；CheckButton / Toggle 展示不同选中状态。", selectionPanelX + 24.0f, kPanelTop + 300.0f, selectionPanelW - 48.0f, 30.0f, 17.0f));
    window->addChild(createLabel(L"RadioButton 通过 RadioGroup 实现互斥选择。", selectionPanelX + 24.0f, kPanelTop + 336.0f, selectionPanelW - 48.0f, 30.0f, 17.0f));

    // ---------------------------------------------------------------------
    // OptionButton / MenuButton / PopupMenu
    // ---------------------------------------------------------------------
    constexpr float menuPanelX = 1120.0f;
    constexpr float menuPanelW = 740.0f;
    window->addChild(createPanel(menuPanelX, kPanelTop, menuPanelW, 350.0f));
    window->addChild(createLabel(L"Menu Controls", menuPanelX + 24.0f, kPanelTop + 22.0f, menuPanelW - 48.0f, 38.0f, 23.0f));

    const float menuLeft = menuPanelX + 24.0f;
    const float menuRight = menuPanelX + 382.0f;
    const float menuWidth = 330.0f;
    window->addChild(createLabel(L"OptionButton", menuLeft, kPanelTop + 76.0f, menuWidth, 30.0f, 18.0f));
    auto optionButton = MROptionButton::create();
    configureMenuButton(optionButton, L"请选择驾驶员", menuLeft, kPanelTop + 116.0f, menuWidth);
    optionButton->addOption(L"驾驶员 A", 101);
    optionButton->addOption(L"驾驶员 B", 102);
    optionButton->addOption(L"访客模式", 103);
    optionButton->getPopupMenu()->setMenuWidth(menuWidth);
    optionButton->setOnSelectedCallback([](int id, const std::wstring&) { LOG_I("OptionButton selected id = {}", id); });
    window->addChild(optionButton);

    window->addChild(createLabel(L"MenuButton", menuRight, kPanelTop + 76.0f, menuWidth, 30.0f, 18.0f));
    auto menuButton = MRMenuButton::create();
    configureMenuButton(menuButton, L"车辆操作", menuRight, kPanelTop + 116.0f, menuWidth);
    menuButton->addMenuItem(L"保存当前设置", 201);
    menuButton->addMenuItem(L"恢复默认设置", 202);
    menuButton->addMenuItem(L"导出配置", 203);
    menuButton->getPopupMenu()->setMenuWidth(menuWidth);
    menuButton->setOnMenuItemSelectedCallback([](int id, const std::wstring&) { LOG_I("MenuButton selected id = {}", id); });
    window->addChild(menuButton);

    auto popupMenu = MRPopupMenu::create();
    popupMenu->setMenuWidth(menuWidth);
    popupMenu->addItem(L"打开诊断页面", 301);
    popupMenu->addItem(L"查看系统信息", 302);
    popupMenu->addItem(L"维护模式（禁用）", 303, false);
    popupMenu->setOnItemSelectedCallback([](int id, const std::wstring&) { LOG_I("PopupMenu selected id = {}", id); });
    popupMenu->attachTo(window);

    auto popupButton = MRButton::create();
    configureMenuButton(popupButton, L"打开独立 PopupMenu", menuLeft, kPanelTop + 245.0f, menuWidth);
    popupButton->setOnClickCallback([popupMenu, popupButton]() {
        if (popupMenu->isOpen()) {
            popupMenu->hide();
        } else {
            popupMenu->popupBelow(popupButton->getScreenSpaceAABB());
        }
    });
    window->addChild(popupButton);

    // ---------------------------------------------------------------------
    // SpinBox / Separator / Spacer / HBoxContainer
    // ---------------------------------------------------------------------
    constexpr float spinTop = 520.0f;
    window->addChild(createPanel(menuPanelX, spinTop, menuPanelW, 420.0f));
    window->addChild(createLabel(L"SpinBox / Separator / Spacer", menuPanelX + 24.0f, spinTop + 22.0f, menuPanelW - 48.0f, 38.0f, 23.0f));

    auto topSeparator = MRHSeparator::create();
    topSeparator->getComponent<Transform>()->setPosition(menuPanelX + 24.0f, spinTop + 78.0f, 0.0f);
    topSeparator->getComponent<Transform>()->setSize(menuPanelW - 48.0f, 2.0f);
    window->addChild(topSeparator);

    window->addChild(createLabel(L"座舱温度", menuPanelX + 24.0f, spinTop + 110.0f, 150.0f, 42.0f));
    auto temperature = MRSpinBox::create();
    temperature->getComponent<Transform>()->setPosition(menuPanelX + 180.0f, spinTop + 104.0f, 0.0f);
    temperature->getComponent<Transform>()->setSize(240.0f, 56.0f);
    temperature->setRange(16.0, 30.0);
    temperature->setStep(0.5);
    temperature->setDecimals(1);
    temperature->setSuffix(L" C");
    temperature->setValue(22.5);
    temperature->setOnValueChangedCallback([](double value) { LOG_I("temperature = {}", value); });
    window->addChild(temperature);

    auto verticalSeparator = MRVSeparator::create();
    verticalSeparator->getComponent<Transform>()->setPosition(menuPanelX + 450.0f, spinTop + 100.0f, 0.0f);
    verticalSeparator->getComponent<Transform>()->setSize(2.0f, 150.0f);
    window->addChild(verticalSeparator);

    window->addChild(createLabel(L"预约时间", menuPanelX + 480.0f, spinTop + 110.0f, 140.0f, 42.0f));
    auto hour = MRSpinBox::create();
    hour->getComponent<Transform>()->setPosition(menuPanelX + 480.0f, spinTop + 158.0f, 0.0f);
    hour->getComponent<Transform>()->setSize(180.0f, 56.0f);
    hour->setRange(0.0, 23.0);
    hour->setStep(1.0);
    hour->setDecimals(0);
    hour->setSuffix(L" h");
    hour->setValue(8.0);
    window->addChild(hour);

    auto middleSeparator = MRHSeparator::create();
    middleSeparator->getComponent<Transform>()->setPosition(menuPanelX + 24.0f, spinTop + 260.0f, 0.0f);
    middleSeparator->getComponent<Transform>()->setSize(menuPanelW - 48.0f, 2.0f);
    window->addChild(middleSeparator);

    window->addChild(createLabel(L"Spacer 弹性占位：按钮自动分布到容器两端", menuPanelX + 24.0f, spinTop + 278.0f, menuPanelW - 48.0f, 32.0f, 17.0f));
    auto actionRow = std::make_shared<HBoxContainer>();
    actionRow->setSpacing(12.0f);
    actionRow->getComponent<Transform>()->setPosition(menuPanelX + 24.0f, spinTop + 326.0f, 0.0f);
    actionRow->getComponent<Transform>()->setSize(menuPanelW - 48.0f, 48.0f);
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

    LOG_I("ControlsDemo started");
    engine->render();
    return 0;
}
