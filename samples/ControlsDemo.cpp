#include "morrow/Engine.h"
#include "morrow/Texture.h"

#include <array>

#include "morrow/base/Transform.h"
#include "morrow/effects/BackdropBlur.h"
#include "morrow/elements/MRButton.h"
#include "morrow/elements/MRCheckBox.h"
#include "morrow/elements/MRCheckButton.h"
#include "morrow/elements/MRColor.h"
#include "morrow/elements/MRLabel.h"
#include "morrow/elements/MRMenuButton.h"
#include "morrow/elements/MROptionButton.h"
#include "morrow/elements/MRPopupMenu.h"
#include "morrow/elements/MRRadioButton.h"
#include "morrow/elements/MRSeparator.h"
#include "morrow/elements/MRSegmentedButton.h"
#include "morrow/elements/MRSpacer.h"
#include "morrow/elements/MRSpinBox.h"
#include "morrow/elements/MRTextureButton.h"
#include "morrow/elements/MRToggle.h"
#include "morrow/layout/HBoxContainer.h"

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
    auto scene = engine->getScene2D();
    engine->setClearColor(0.94f, 0.96f, 0.99f, 1.0f);
    engine->addFonts({FontInfo{
        .name = "default",
        .path = "assets/fonts/MorrowSansCN1.1-Regular.otf",
    }});

    scene->addChild(createLabel(L"Controls Showcase：按钮、选择、菜单、数值输入与布局", 60.0f, 30.0f, 1700.0f, 54.0f, 34.0f));
    scene->addChild(createLabel(L"统一展示常用交互控件及其回调、状态和容器组合方式。", 60.0f, 84.0f, 1700.0f, 36.0f, 20.0f));

    // ---------------------------------------------------------------------
    // Button / TextureButton / HBoxContainer
    // ---------------------------------------------------------------------
    constexpr float buttonPanelX = 60.0f;
    constexpr float buttonPanelW = 420.0f;
    scene->addChild(createPanel(buttonPanelX, kPanelTop, buttonPanelW, 800.0f));
    scene->addChild(createLabel(L"Button / TextureButton", buttonPanelX + 24.0f, kPanelTop + 22.0f, buttonPanelW - 48.0f, 38.0f, 23.0f));

    auto buttonRow = std::make_shared<HBoxContainer>();
    buttonRow->setSpacing(12.0f);
    buttonRow->getComponent<Transform>()->setPosition(buttonPanelX + 24.0f, kPanelTop + 88.0f, 0.0f);
    buttonRow->getComponent<Transform>()->setSize(buttonPanelW - 48.0f, 58.0f);

    auto textButton = MRButton::create();
    configureButton(textButton, L"普通按钮", 0.0f, 0.0f, 180.0f);
    auto textButtonClickConnection =
        textButton->events().onClicked.connect(
            [](BaseButton&) { LOG_I("normal button clicked"); });
    buttonRow->addChild(textButton);

    auto colorButton = MRButton::create();
    configureButton(colorButton, L"彩色按钮", 0.0f, 0.0f, 180.0f);
    colorButton->setBackgroundColor(0.9f, 0.35f, 0.28f, 1.0f);
    colorButton->setHoverColor(Vector4(0.96f, 0.48f, 0.38f, 1.0f));
    colorButton->setTextColor(1.0f, 1.0f, 1.0f, 1.0f);
    buttonRow->addChild(colorButton);
    scene->addChild(buttonRow);

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
    auto textureButtonClickConnection =
        textureButton->events().onClicked.connect(
            [](BaseButton&) { LOG_I("texture button clicked"); });
    textureButton->getComponent<Transform>()->setPosition(buttonPanelX + 110.0f, kPanelTop + 190.0f, 0.0f);
    textureButton->getComponent<Transform>()->setSize(200.0f, 200.0f);
    scene->addChild(textureButton);
    scene->addChild(createLabel(L"MRTextureButton\nNormal / Hover / Pressed / Disabled", buttonPanelX + 30.0f, kPanelTop + 405.0f, buttonPanelW - 60.0f, 80.0f, 18.0f,
                                 HorizontalAlignment::CENTER));

    // ---------------------------------------------------------------------
    // CheckBox / CheckButton / Toggle / RadioButton
    // ---------------------------------------------------------------------
    constexpr float selectionPanelX = 520.0f;
    constexpr float selectionPanelW = 540.0f;
    // 面板下沉到模糊源（displayLayer -5 < 玻璃边界 0）：面板与条纹都会被
    // 玻璃采样模糊，且不会在回屏后盖住模糊面片
    auto selectionPanel = createPanel(selectionPanelX, kPanelTop, selectionPanelW, 800.0f);
    selectionPanel->setDisplayLayer(-5);
    scene->addChild(selectionPanel);
    scene->addChild(createLabel(L"Selection Controls", selectionPanelX + 24.0f, kPanelTop + 22.0f, selectionPanelW - 48.0f, 38.0f, 23.0f));
    scene->addChild(createLabel(L"多选与开关", selectionPanelX + 24.0f, kPanelTop + 82.0f, 230.0f, 32.0f, 18.0f));
    scene->addChild(createLabel(L"RadioGroup 单选", selectionPanelX + 278.0f, kPanelTop + 82.0f, 230.0f, 32.0f, 18.0f));

    const float selectionLeft = selectionPanelX + 24.0f;
    const float selectionRight = selectionPanelX + 278.0f;
    const float selectionY = kPanelTop + 130.0f;

    auto checkBox = MRCheckBox::create();
    configureSelectionControl(checkBox, L"启用空调", selectionLeft, selectionY);
    auto checkBoxConnection =
        checkBox->selectionEvents().onCheckedChanged.connect(
            [](MRSelectableButton&, bool checked) {
                LOG_I("CheckBox checked = {}", checked);
            });
    scene->addChild(checkBox);

    auto checkButton = MRCheckButton::create();
    configureSelectionControl(checkButton, L"座椅加热", selectionLeft, selectionY + 62.0f);
    checkButton->setChecked(true);
    auto checkButtonConnection =
        checkButton->selectionEvents().onCheckedChanged.connect(
            [](MRSelectableButton&, bool checked) {
                LOG_I("CheckButton checked = {}", checked);
            });
    scene->addChild(checkButton);

    auto toggle = MRToggle::create();
    configureSelectionControl(toggle, L"自动大灯", selectionLeft, selectionY + 124.0f);
    auto toggleConnection =
        toggle->selectionEvents().onCheckedChanged.connect(
            [](MRSelectableButton&, bool checked) {
                LOG_I("Toggle value = {}", checked);
            });
    scene->addChild(toggle);

    auto group = std::make_shared<MRRadioGroup>();
    auto radioA = MRRadioButton::create();
    configureSelectionControl(radioA, L"舒适模式", selectionRight, selectionY);
    radioA->setGroup(group);
    auto radioAConnection = radioA->selectionEvents().onCheckedChanged.connect([](MRSelectableButton&, bool checked) {
        if (checked)
            LOG_I("Radio selection = comfort");
    });
    scene->addChild(radioA);

    auto radioB = MRRadioButton::create();
    configureSelectionControl(radioB, L"运动模式", selectionRight, selectionY + 62.0f);
    radioB->setGroup(group);
    auto radioBConnection = radioB->selectionEvents().onCheckedChanged.connect([](MRSelectableButton&, bool checked) {
        if (checked)
            LOG_I("Radio selection = sport");
    });
    scene->addChild(radioB);

    auto radioC = MRRadioButton::create();
    configureSelectionControl(radioC, L"节能模式", selectionRight, selectionY + 124.0f);
    radioC->setGroup(group);
    auto radioCConnection = radioC->selectionEvents().onCheckedChanged.connect([](MRSelectableButton&, bool checked) {
        if (checked)
            LOG_I("Radio selection = eco");
    });
    scene->addChild(radioC);
    group->select(radioA);

    scene->addChild(
        createLabel(L"CheckBox 支持多选；CheckButton / Toggle 展示不同选中状态。", selectionPanelX + 24.0f, kPanelTop + 300.0f, selectionPanelW - 48.0f, 30.0f, 17.0f));
    scene->addChild(createLabel(L"RadioButton 通过 RadioGroup 实现互斥选择。", selectionPanelX + 24.0f, kPanelTop + 336.0f, selectionPanelW - 48.0f, 30.0f, 17.0f));

    scene->addChild(createLabel(L"MRSegmentedButton 分段选择", selectionPanelX + 24.0f, kPanelTop + 392.0f, selectionPanelW - 48.0f, 30.0f, 18.0f));
    // 玻璃背后的彩色内容带：与分段按钮同矩形，displayLayer(-1) 沉入模糊源
    //（严格小于玻璃边界 0 才会被采样，见 BackdropBlurManager 分段语义），
    // 压在 -5 的面板之上
    const std::array<Vector4, 6> stripeColors = {{
        Vector4(0.91f, 0.36f, 0.32f, 1.0f),
        Vector4(0.95f, 0.60f, 0.28f, 1.0f),
        Vector4(0.94f, 0.80f, 0.32f, 1.0f),
        Vector4(0.36f, 0.68f, 0.52f, 1.0f),
        Vector4(0.32f, 0.52f, 0.82f, 1.0f),
        Vector4(0.62f, 0.40f, 0.76f, 1.0f),
    }};
    const float stripeW = (selectionPanelW - 48.0f) / static_cast<float>(stripeColors.size());
    for (size_t i = 0; i < stripeColors.size(); ++i) {
        auto stripe = MRColor::create();
        stripe->setColor(stripeColors[i]);
        stripe->getComponent<Transform>()->setPosition(selectionPanelX + 24.0f + stripeW * static_cast<float>(i), kPanelTop + 432.0f, 0.0f);
        stripe->getComponent<Transform>()->setSize(stripeW, 56.0f);
        stripe->setDisplayLayer(-1);
        scene->addChild(stripe);
    }
    // 毛玻璃底：透明 MRColor 作属主挂 BackdropBlur（模糊面片即背景，
    // 圆角跟随属主的 setRounding），MRSegmentedButton 作为纯容器叠其上
    auto segmentedGlass = MRColor::create();
    segmentedGlass->setColor(0.0f, 0.0f, 0.0f, 0.0f);
    // segmentedGlass->setRounding(12.0f);
    segmentedGlass->getComponent<Transform>()->setPosition(selectionPanelX + 24.0f, kPanelTop + 432.0f, 0.0f);
    segmentedGlass->getComponent<Transform>()->setSize(selectionPanelW - 48.0f, 56.0f);
    auto glassBlur = segmentedGlass->addComponent<BackdropBlur>();
    glassBlur->setBlurRadius(30.0f);
    glassBlur->setTintColor(0.97f, 0.98f, 1.0f, 0.45f);
    scene->addChild(segmentedGlass);
    auto segmented = MRSegmentedButton::create();
    segmented->getComponent<Transform>()->setPosition(selectionPanelX + 24.0f, kPanelTop + 432.0f, 0.0f);
    segmented->getComponent<Transform>()->setSize(selectionPanelW - 48.0f, 56.0f);
    segmented->addSegment(L"标准");
    segmented->addSegment(L"柔和");
    segmented->addSegment(L"运动");
    segmented->setOnSelectionChanged([](int index) { LOG_I("segmented selection = {}", index); });
    scene->addChild(segmented);
    scene->addChild(
        createLabel(L"N 选一分段按钮：选中段叠加程序生成的点状背景并高亮文字。\n组件为纯容器；毛玻璃背景 = 透明 MRColor 属主挂 BackdropBlur + 分段按钮叠上\n（层级：内容 -1 沉入模糊源，玻璃属主 0 为分段边界）。",
                    selectionPanelX + 24.0f, kPanelTop + 506.0f, selectionPanelW - 48.0f, 78.0f, 17.0f));

    // ---------------------------------------------------------------------
    // OptionButton / MenuButton / PopupMenu
    // ---------------------------------------------------------------------
    constexpr float menuPanelX = 1120.0f;
    constexpr float menuPanelW = 740.0f;
    scene->addChild(createPanel(menuPanelX, kPanelTop, menuPanelW, 350.0f));
    scene->addChild(createLabel(L"Menu Controls", menuPanelX + 24.0f, kPanelTop + 22.0f, menuPanelW - 48.0f, 38.0f, 23.0f));

    const float menuLeft = menuPanelX + 24.0f;
    const float menuRight = menuPanelX + 382.0f;
    const float menuWidth = 330.0f;
    scene->addChild(createLabel(L"OptionButton", menuLeft, kPanelTop + 76.0f, menuWidth, 30.0f, 18.0f));
    auto optionButton = MROptionButton::create();
    configureMenuButton(optionButton, L"请选择驾驶员", menuLeft, kPanelTop + 116.0f, menuWidth);
    optionButton->addOption(L"驾驶员 A", 101);
    optionButton->addOption(L"驾驶员 B", 102);
    optionButton->addOption(L"访客模式", 103);
    optionButton->getPopupMenu()->setMenuWidth(menuWidth);
    auto optionConnection =
        optionButton->selectionEvents().onSelected.connect(
            [](MROptionButton&, int id, const std::wstring&) {
                LOG_I("OptionButton selected id = {}", id);
            });
    scene->addChild(optionButton);

    scene->addChild(createLabel(L"MenuButton", menuRight, kPanelTop + 76.0f, menuWidth, 30.0f, 18.0f));
    auto menuButton = MRMenuButton::create();
    configureMenuButton(menuButton, L"车辆操作", menuRight, kPanelTop + 116.0f, menuWidth);
    menuButton->addMenuItem(L"保存当前设置", 201);
    menuButton->addMenuItem(L"恢复默认设置", 202);
    menuButton->addMenuItem(L"导出配置", 203);
    menuButton->getPopupMenu()->setMenuWidth(menuWidth);
    auto menuConnection =
        menuButton->menuEvents().onItemSelected.connect(
            [](MRMenuButton&, int id, const std::wstring&) {
                LOG_I("MenuButton selected id = {}", id);
            });
    scene->addChild(menuButton);

    auto popupMenu = MRPopupMenu::create();
    popupMenu->setMenuWidth(menuWidth);
    popupMenu->addItem(L"打开诊断页面", 301);
    popupMenu->addItem(L"查看系统信息", 302);
    popupMenu->addItem(L"维护模式（禁用）", 303, false);
    auto popupMenuConnection =
        popupMenu->events().onItemSelected.connect(
            [](MRPopupMenu&, int id, const std::wstring&) {
                LOG_I("PopupMenu selected id = {}", id);
            });
    popupMenu->attachTo(scene);

    auto popupButton = MRButton::create();
    configureMenuButton(popupButton, L"打开独立 PopupMenu", menuLeft, kPanelTop + 245.0f, menuWidth);
    auto popupButtonConnection = popupButton->events().onClicked.connect([popupMenu, popupButton](BaseButton&) {
        if (popupMenu->isOpen()) {
            popupMenu->hide();
        } else {
            popupMenu->popupBelow(popupButton->getWorldSpaceAABB());
        }
    });
    scene->addChild(popupButton);

    // ---------------------------------------------------------------------
    // SpinBox / Separator / Spacer / HBoxContainer
    // ---------------------------------------------------------------------
    constexpr float spinTop = 520.0f;
    scene->addChild(createPanel(menuPanelX, spinTop, menuPanelW, 420.0f));
    scene->addChild(createLabel(L"SpinBox / Separator / Spacer", menuPanelX + 24.0f, spinTop + 22.0f, menuPanelW - 48.0f, 38.0f, 23.0f));

    auto topSeparator = MRHSeparator::create();
    topSeparator->getComponent<Transform>()->setPosition(menuPanelX + 24.0f, spinTop + 78.0f, 0.0f);
    topSeparator->getComponent<Transform>()->setSize(menuPanelW - 48.0f, 2.0f);
    scene->addChild(topSeparator);

    scene->addChild(createLabel(L"座舱温度", menuPanelX + 24.0f, spinTop + 110.0f, 150.0f, 42.0f));
    auto temperature = MRSpinBox::create();
    temperature->getComponent<Transform>()->setPosition(menuPanelX + 180.0f, spinTop + 104.0f, 0.0f);
    temperature->getComponent<Transform>()->setSize(240.0f, 56.0f);
    temperature->setRange(16.0, 30.0);
    temperature->setStep(0.5);
    temperature->setDecimals(1);
    temperature->setSuffix(L" C");
    temperature->setValue(22.5);
    auto temperatureConnection =
        temperature->events().onValueChanged.connect(
            [](MRSpinBox&, double value) { LOG_I("temperature = {}", value); });
    scene->addChild(temperature);

    auto verticalSeparator = MRVSeparator::create();
    verticalSeparator->getComponent<Transform>()->setPosition(menuPanelX + 450.0f, spinTop + 100.0f, 0.0f);
    verticalSeparator->getComponent<Transform>()->setSize(2.0f, 150.0f);
    scene->addChild(verticalSeparator);

    scene->addChild(createLabel(L"预约时间", menuPanelX + 480.0f, spinTop + 110.0f, 140.0f, 42.0f));
    auto hour = MRSpinBox::create();
    hour->getComponent<Transform>()->setPosition(menuPanelX + 480.0f, spinTop + 158.0f, 0.0f);
    hour->getComponent<Transform>()->setSize(180.0f, 56.0f);
    hour->setRange(0.0, 23.0);
    hour->setStep(1.0);
    hour->setDecimals(0);
    hour->setSuffix(L" h");
    hour->setValue(8.0);
    scene->addChild(hour);

    auto middleSeparator = MRHSeparator::create();
    middleSeparator->getComponent<Transform>()->setPosition(menuPanelX + 24.0f, spinTop + 260.0f, 0.0f);
    middleSeparator->getComponent<Transform>()->setSize(menuPanelW - 48.0f, 2.0f);
    scene->addChild(middleSeparator);

    scene->addChild(createLabel(L"Spacer 弹性占位：按钮自动分布到容器两端", menuPanelX + 24.0f, spinTop + 278.0f, menuPanelW - 48.0f, 32.0f, 17.0f));
    auto actionRow = std::make_shared<HBoxContainer>();
    actionRow->setSpacing(12.0f);
    actionRow->getComponent<Transform>()->setPosition(menuPanelX + 24.0f, spinTop + 326.0f, 0.0f);
    actionRow->getComponent<Transform>()->setSize(menuPanelW - 48.0f, 48.0f);
    auto cancel = createActionButton(L"取消");
    auto spacer = MRSpacer::create(1.0f);
    spacer->setMinimumSize(Vector2(24.0f, 1.0f));
    auto apply = createActionButton(L"应用设置");
    auto cancelConnection =
        cancel->events().onClicked.connect(
            [](BaseButton&) { LOG_I("cancel settings"); });
    auto applyConnection =
        apply->events().onClicked.connect(
            [](BaseButton&) { LOG_I("apply settings"); });
    actionRow->addChild(cancel);
    actionRow->addChild(spacer);
    actionRow->addChild(apply);
    scene->addChild(actionRow);

    LOG_I("ControlsDemo started");
    engine->render();
    return 0;
}
