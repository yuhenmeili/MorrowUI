#include "Engine.h"
#include "FontManager.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "elements/MRColor.h"
#include "elements/MRLabel.h"
#include "elements/MRParallaxBackground.h"
#include "elements/MRPopup.h"

using namespace morrow;

namespace {

std::shared_ptr<MRLabel> label(const std::wstring& text, float x, float y, float w, float h, float size) {
    auto result = std::make_shared<MRLabel>();
    result->setText(text, "default");
    result->setFontSize(size);
    result->setFontColor(0.9f, 0.94f, 1.0f, 1.0f);
    result->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    result->getComponent<Transform>()->setPosition(x, y, 0.0f);
    result->getComponent<Transform>()->setSize(w, h);
    return result;
}

std::shared_ptr<MRButton> button(const std::wstring& text, float x, float y) {
    auto result = MRButton::create();
    result->setText(text, "default");
    result->setTextFontSize(20.0f);
    result->setTextColor(0.1f, 0.15f, 0.22f, 1.0f);
    result->setBackgroundColor(0.86f, 0.92f, 0.98f, 1.0f);
    result->setHoverColor(Vector4(0.76f, 0.85f, 0.95f, 1.0f));
    result->setPressedColor(Vector4(0.66f, 0.78f, 0.91f, 1.0f));
    result->setCornerRadius(6.0f);
    result->getComponent<Transform>()->setPosition(x, y, 0.0f);
    result->getComponent<Transform>()->setSize(250.0f, 52.0f);
    return result;
}

MRParallax2DSharedPtr layer(float scale, const Vector4& color, float y, float height) {
    auto result = MRParallax2D::create();
    result->setBasePosition(0.0f, y);
    auto fill = MRColor::create();
    fill->setColor(color);
    fill->getComponent<Transform>()->setPosition(0.0f, 0.0f, 0.0f);
    fill->getComponent<Transform>()->setSize(1920.0f, height);
    result->addChild(fill);
    result->setScrollScale(scale, scale);
    return result;
}

}  // namespace

int main() {
    EngineOptions options;
    // options.windowInfo.width = 1280;
    // options.windowInfo.height = 720;
    auto engine = std::make_shared<Engine>(options);
    auto window = engine->getWindow();
    window->setClearColor(0.04f, 0.07f, 0.13f, 1.0f);
    engine->addFonts({FontInfo{.name = "default", .path = "assets/fonts/MorrowSansCN1.1-Regular.otf"}});

    auto background = MRParallaxBackground::create();
    background->getComponent<Transform>()->setSize(1280.0f, 720.0f);
    background->addLayer(layer(0.12f, Vector4(0.08f, 0.16f, 0.28f, 1.0f), 110.0f, 220.0f), 0.12f, 0.12f);
    background->addLayer(layer(0.35f, Vector4(0.12f, 0.32f, 0.48f, 1.0f), 330.0f, 180.0f), 0.35f, 0.35f);
    background->addLayer(layer(0.72f, Vector4(0.16f, 0.52f, 0.56f, 1.0f), 510.0f, 180.0f), 0.72f, 0.72f);
    window->addChild(background);

    window->addChild(label(L"ParallaxBackground / Parallax2D", 50.0f, 42.0f, 700.0f, 46.0f, 30.0f));
    window->addChild(label(L"滚动偏移会按不同倍率移动每个背景层", 50.0f, 88.0f, 620.0f, 38.0f, 18.0f));

    auto left = button(L"向左滚动", 50.0f, 610.0f);
    left->setOnClickCallback([background]() {
        auto offset = background->getScrollOffset();
        background->setScrollOffset(offset.x - 60.0f, offset.y);
    });
    window->addChild(left);

    auto right = button(L"向右滚动", 320.0f, 610.0f);
    right->setOnClickCallback([background]() {
        auto offset = background->getScrollOffset();
        background->setScrollOffset(offset.x + 60.0f, offset.y);
    });
    window->addChild(right);

    auto tooltip = MRTooltip::create();
    tooltip->setText(L"Tooltip：这是跟随目标区域定位的提示气泡。");
    tooltip->attachTo(window);
    auto tooltipButton = button(L"显示 Tooltip", 590.0f, 610.0f);
    tooltipButton->setOnClickCallback([tooltip, tooltipButton]() {
        tooltip->showFor(tooltipButton->getScreenSpaceAABB());
    });
    window->addChild(tooltipButton);

    auto dialog = MRDialog::create();
    dialog->setTitle(L"确认操作");
    dialog->setMessage(L"这是一个可复用的 Dialog。点击确认或取消都会关闭弹窗，并触发对应回调。");
    dialog->setOnConfirmedCallback([]() { LOG_I("Dialog confirmed"); });
    dialog->setOnCanceledCallback([]() { LOG_I("Dialog canceled"); });
    dialog->attachTo(window);
    auto dialogButton = button(L"打开 Dialog", 860.0f, 610.0f);
    dialogButton->setOnClickCallback([dialog]() {
        dialog->popup(380.0f, 210.0f);
    });
    window->addChild(dialogButton);

    LOG_I("Parallax and popup demo started");
    engine->render();
    return 0;
}
