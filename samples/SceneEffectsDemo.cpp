#include <array>

#include "Engine.h"
#include "FontManager.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "elements/MRCanvasModulate.h"
#include "elements/MRColor.h"
#include "elements/MRLabel.h"
#include "elements/MRParallaxBackground.h"
#include "elements/MRPopup.h"

using namespace morrow;

namespace {

std::shared_ptr<MRLabel> createLabel(const std::wstring& text, float x, float y, float width, float height, float fontSize = 20.0f,
                                     const Vector4& color = Vector4(0.9f, 0.94f, 1.0f, 1.0f)) {
    auto result = std::make_shared<MRLabel>();
    result->setText(text, "default");
    result->setFontSize(fontSize);
    result->setFontColor(color);
    result->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    result->getComponent<Transform>()->setPosition(x, y, 0.0f);
    result->getComponent<Transform>()->setSize(width, height);
    return result;
}

std::shared_ptr<MRButton> createButton(const std::wstring& text, float x, float y, float width = 250.0f) {
    auto result = MRButton::create();
    result->setText(text, "default");
    result->setTextFontSize(19.0f);
    result->setTextColor(0.1f, 0.15f, 0.22f, 1.0f);
    result->setBackgroundColor(0.86f, 0.92f, 0.98f, 1.0f);
    result->setHoverColor(Vector4(0.76f, 0.85f, 0.95f, 1.0f));
    result->setPressedColor(Vector4(0.66f, 0.78f, 0.91f, 1.0f));
    result->setCornerRadius(7.0f);
    result->getComponent<Transform>()->setPosition(x, y, 0.0f);
    result->getComponent<Transform>()->setSize(width, 52.0f);
    return result;
}

MRParallax2DSharedPtr createParallaxLayer(float scale, const Vector4& color, float y, float height) {
    auto result = MRParallax2D::create();
    result->setBasePosition(0.0f, y);
    auto fill = MRColor::create();
    fill->setColor(color);
    fill->getComponent<Transform>()->setPosition(0.0f, 0.0f, 0.0f);
    fill->getComponent<Transform>()->setSize(1280.0f, height);
    result->addChild(fill);
    result->setScrollScale(scale, scale);
    return result;
}

}  // namespace

int main() {
    EngineOptions options;
    options.windowInfo.name = "SceneEffectsDemo";
    auto engine = std::make_shared<Engine>(options);
    auto window = engine->getWindow();
    window->setClearColor(0.04f, 0.07f, 0.13f, 1.0f);
    engine->addFonts({FontInfo{
        .name = "default",
        .path = "assets/fonts/MorrowSansCN1.1-Regular.otf",
    }});

    // ---------------------------------------------------------------------
    // ParallaxBackground / Parallax2D
    // ---------------------------------------------------------------------
    auto background = MRParallaxBackground::create();
    background->getComponent<Transform>()->setSize(1280.0f, 720.0f);
    background->addLayer(createParallaxLayer(0.12f, Vector4(0.08f, 0.16f, 0.28f, 1.0f), 110.0f, 220.0f), 0.12f, 0.12f);
    background->addLayer(createParallaxLayer(0.35f, Vector4(0.12f, 0.32f, 0.48f, 1.0f), 330.0f, 180.0f), 0.35f, 0.35f);
    background->addLayer(createParallaxLayer(0.72f, Vector4(0.16f, 0.52f, 0.56f, 1.0f), 510.0f, 180.0f), 0.72f, 0.72f);
    window->addChild(background);

    window->addChild(createLabel(L"ParallaxBackground / Parallax2D", 50.0f, 42.0f, 700.0f, 46.0f, 30.0f));
    window->addChild(createLabel(L"不同背景层使用不同滚动倍率", 50.0f, 88.0f, 620.0f, 38.0f, 18.0f));

    auto scrollLeft = createButton(L"向左滚动", 50.0f, 610.0f);
    auto scrollLeftConnection = scrollLeft->events().onClicked.connect([background](BaseButton&) {
        const auto offset = background->getScrollOffset();
        background->setScrollOffset(offset.x - 60.0f, offset.y);
    });
    window->addChild(scrollLeft);

    auto scrollRight = createButton(L"向右滚动", 320.0f, 610.0f);
    auto scrollRightConnection = scrollRight->events().onClicked.connect([background](BaseButton&) {
        const auto offset = background->getScrollOffset();
        background->setScrollOffset(offset.x + 60.0f, offset.y);
    });
    window->addChild(scrollRight);

    // ---------------------------------------------------------------------
    // CanvasModulate
    // ---------------------------------------------------------------------
    window->addChild(createLabel(L"MRCanvasModulate", 1330.0f, 42.0f, 500.0f, 46.0f, 30.0f));
    window->addChild(createLabel(L"全局色调调制会同时影响背景、色块、文字和按钮", 1330.0f, 88.0f, 520.0f, 58.0f, 18.0f));

    const std::array<Vector4, 3> cardColors = {
        Vector4(1.0f, 0.62f, 0.24f, 1.0f),
        Vector4(0.22f, 0.78f, 0.48f, 1.0f),
        Vector4(0.62f, 0.4f, 0.9f, 1.0f),
    };
    const std::array<std::wstring, 3> cardNames = {
        L"暖色",
        L"状态",
        L"氛围",
    };
    for (int index = 0; index < 3; ++index) {
        const float x = 1320.0f + static_cast<float>(index % 2) * 260.0f;
        const float y = 180.0f + static_cast<float>(index / 2) * 180.0f;
        auto card = MRColor::create();
        card->setColor(cardColors[index]);
        card->setRounding(16.0f);
        card->getComponent<Transform>()->setPosition(x, y, 0.0f);
        card->getComponent<Transform>()->setSize(220.0f, 140.0f);
        window->addChild(card);
        window->addChild(createLabel(cardNames[index], x + 20.0f, y + 48.0f, 180.0f, 42.0f, 24.0f, Vector4(0.08f, 0.12f, 0.18f, 1.0f)));
    }

    auto modulate = MRCanvasModulate::create();
    modulate->setModulateColor(Vector4(0.38f, 0.48f, 0.68f, 1.0f));
    modulate->setStrength(0.0f);
    window->addChild(modulate);

    auto nightButton = createButton(L"切换夜间模式", 1330.0f, 570.0f, 460.0f);
    auto nightModeConnection = nightButton->events().onClicked.connect([modulate](BaseButton&) {
        modulate->setNightMode(!modulate->isNightMode());
        LOG_I("night mode = {}", modulate->isNightMode());
    });
    window->addChild(nightButton);

    // ---------------------------------------------------------------------
    // Tooltip / Dialog
    // ---------------------------------------------------------------------
    auto tooltip = MRTooltip::create();
    tooltip->setText(L"Tooltip：跟随目标区域定位的提示气泡。");
    tooltip->attachTo(window);
    auto tooltipButton = createButton(L"显示 Tooltip", 590.0f, 610.0f);
    auto tooltipConnection = tooltipButton->events().onClicked.connect([tooltip, tooltipButton](BaseButton&) {
        tooltip->showFor(tooltipButton->getScreenSpaceAABB());
    });
    window->addChild(tooltipButton);

    auto dialog = MRDialog::create();
    dialog->setTitle(L"确认操作");
    dialog->setMessage(L"这是一个可复用的 Dialog。确认或取消都会关闭弹窗。");
    auto dialogConfirmedConnection =
        dialog->dialogEvents().onConfirmed.connect(
            [](MRDialog&) { LOG_I("Dialog confirmed"); });
    auto dialogCanceledConnection =
        dialog->dialogEvents().onCanceled.connect(
            [](MRDialog&) { LOG_I("Dialog canceled"); });
    dialog->attachTo(window);
    auto dialogButton = createButton(L"打开 Dialog", 860.0f, 610.0f);
    auto dialogButtonConnection =
        dialogButton->events().onClicked.connect(
            [dialog](BaseButton&) { dialog->popup(380.0f, 210.0f); });
    window->addChild(dialogButton);

    LOG_I("SceneEffectsDemo started");
    engine->render();
    return 0;
}
