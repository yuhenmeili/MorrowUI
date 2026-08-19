#include "Engine.h"
#include "FontManager.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "elements/MRColor.h"
#include "elements/MRLabel.h"
#include "elements/MRScrollContainer.h"

using namespace morrow;

namespace {

std::shared_ptr<MRLabel> createTitle(const std::wstring& text, float x, float y, float width) {
    auto label = std::make_shared<MRLabel>();
    label->setText(text, "default");
    label->setFontSize(28.0f);
    label->setFontColor(0.12f, 0.15f, 0.2f, 1.0f);
    label->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    label->getComponent<Transform>()->setPosition(x, y, 0.0f);
    label->getComponent<Transform>()->setSize(width, 44.0f);
    return label;
}

std::shared_ptr<MRButton> createListItem(int index, float width, float y) {
    auto item = MRButton::create();
    item->setText(L"车辆消息 " + std::to_wstring(index), "default");
    item->setTextFontSize(22.0f);
    item->setTextColor(0.1f, 0.13f, 0.18f, 1.0f);
    item->setTextAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    item->setBackgroundColor(index % 2 == 0 ? Vector4(0.91f, 0.94f, 0.97f, 1.0f) : Vector4(0.96f, 0.97f, 0.99f, 1.0f));
    item->setHoverColor(Vector4(0.78f, 0.87f, 0.94f, 1.0f));
    item->setPressedColor(Vector4(0.67f, 0.8f, 0.9f, 1.0f));
    item->setCornerRadius(6.0f);
    item->getComponent<Transform>()->setPosition(0.0f, y, 0.0f);
    item->getComponent<Transform>()->setSize(width, 52.0f);
    item->setOnClickCallback([index]() { LOG_I("Scroll list item clicked: {}", index); });
    return item;
}

}  // namespace

int main() {
    auto engine = std::make_shared<Engine>();
    auto window = engine->getWindow();
    window->setClearColor(0.96f, 0.97f, 0.99f, 1.0f);

    FontInfo fontInfo = {.name = "default", .path = "assets/fonts/MorrowSansCN1.1-Regular.otf"};
    engine->addFonts({fontInfo});

    constexpr float panelX = 140.0f;
    constexpr float panelY = 120.0f;
    constexpr float panelWidth = 640.0f;
    constexpr float viewportHeight = 464.0f;
    constexpr float rowStride = 58.0f;

    window->addChild(createTitle(L"ScrollContainer 长列表", panelX, 56.0f, panelWidth));

    auto panel = MRColor::create();
    panel->setColor(1.0f, 1.0f, 1.0f, 1.0f);
    panel->setRounding(8.0f);
    panel->getComponent<Transform>()->setPosition(panelX - 12.0f, panelY - 12.0f, 0.0f);
    panel->getComponent<Transform>()->setSize(panelWidth + 24.0f, viewportHeight + 24.0f);
    window->addChild(panel);

    auto scroll = MRScrollContainer::create();
    scroll->getComponent<Transform>()->setPosition(panelX, panelY, 0.0f);
    scroll->getComponent<Transform>()->setSize(panelWidth, viewportHeight);
    scroll->setScrollStep(rowStride);
    scroll->getVerticalScrollBar()->setTrackColor(Vector4(0.86f, 0.89f, 0.93f, 1.0f));
    scroll->getVerticalScrollBar()->setThumbColor(Vector4(0.24f, 0.52f, 0.7f, 1.0f));

    for (int index = 1; index <= 24; ++index) {
        scroll->addScrollChild(createListItem(index, panelWidth - 32.0f, static_cast<float>(index - 1) * rowStride));
    }
    window->addChild(scroll);

    auto hint = std::make_shared<MRLabel>();
    hint->setText(L"使用鼠标滚轮、触摸拖动或右侧滚动条浏览列表", "default");
    hint->setFontSize(20.0f);
    hint->setFontColor(0.35f, 0.39f, 0.45f, 1.0f);
    hint->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    hint->getComponent<Transform>()->setPosition(panelX, panelY + viewportHeight + 24.0f, 0.0f);
    hint->getComponent<Transform>()->setSize(700.0f, 36.0f);
    window->addChild(hint);

    LOG_I("ScrollContainer demo started");
    engine->render();
    return 0;
}
