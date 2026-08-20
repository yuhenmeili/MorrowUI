#include "Engine.h"
#include "FontManager.h"
#include "base/Transform.h"
#include "elements/MRColor.h"
#include "elements/MRLabel.h"
#include "elements/MRLineEdit.h"
#include "elements/MRRichTextLabel.h"
#include "elements/MRTextEdit.h"
//
// Created by 0060328 on 25-10-9.
//
using namespace morrow;

namespace {

std::shared_ptr<MRLabel> createLabel(
    const std::wstring& text,
    const std::string& fontName,
    float x,
    float y,
    float width,
    float height,
    float fontSize,
    HorizontalAlignment horizontal = HorizontalAlignment::LEFT,
    VerticalAlignment vertical = VerticalAlignment::CENTER) {
    auto label = std::make_shared<MRLabel>();
    label->setText(text, fontName);
    label->setFontSize(fontSize);
    label->setFontColor(0.12f, 0.16f, 0.22f, 1.0f);
    label->setAlign(horizontal, vertical);
    label->getComponent<Transform>()->setPosition(x, y, 0.0f);
    label->getComponent<Transform>()->setSize(width, height);
    return label;
}

void addAlignmentSample(
    const WindowSharedPtr& window,
    const std::string& fontName,
    const std::wstring& text,
    float x,
    float y,
    HorizontalAlignment horizontal,
    VerticalAlignment vertical) {
    constexpr float width = 280.0f;
    constexpr float height = 105.0f;

    auto background = MRColor::create();
    background->setColor(0.92f, 0.95f, 0.98f, 1.0f);
    background->setRounding(8.0f);
    background->getComponent<Transform>()->setPosition(x, y, -0.1f);
    background->getComponent<Transform>()->setSize(width, height);
    window->addChild(background);

    auto label = createLabel(text, fontName, x, y, width, height, 20.0f, horizontal, vertical);
    window->addChild(label);
}

}  // namespace

int main() {
    EngineOptions engineOptions;
    // engineOptions.multithread = false;
    EngineSharedPtr engine = std::make_shared<Engine>(engineOptions);

    auto window = engine->getWindow();
    window->setClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    FontInfo fontInfo = {.name = "debug_morrow_20", .path = "assets/fonts/MorrowSansCN1.1-Regular.otf"};
    engine->addFonts({fontInfo});

    const std::string fontName = fontInfo.name;

    window->addChild(createLabel(L"文本输入组件", fontName, 120.0f, 90.0f, 620.0f, 48.0f, 30.0f));

    auto multiLineEdit = MRTextEdit::create();
    multiLineEdit->getComponent<Transform>()->setPosition(120.0f, 160.0f, 0.0f);
    multiLineEdit->getComponent<Transform>()->setSize(620.0f, 260.0f);
    multiLineEdit->setFontName(fontName);
    multiLineEdit->setFontSize(24.0f);
    multiLineEdit->setPlaceholder(L"输入多行备注，按 Enter 换行");
    multiLineEdit->setOnTextChangedCallback([](const std::wstring& text) { LOG_I("multi-line text length = {}", text.size()); });
    window->addChild(multiLineEdit);

    auto userNameEdit = MRLineEdit::create();
    userNameEdit->getComponent<Transform>()->setPosition(120.0f, 470.0f, 0.0f);
    userNameEdit->getComponent<Transform>()->setSize(620.0f, 58.0f);
    userNameEdit->setFontName(fontName);
    userNameEdit->setFontSize(23.0f);
    userNameEdit->setPlaceholder(L"用户名（单行，按 Enter 提交）");
    userNameEdit->setMaxLength(24);
    userNameEdit->setOnSubmitCallback([](const std::wstring& text) { LOG_I("user name submitted, length = {}", text.size()); });
    window->addChild(userNameEdit);

    auto passwordEdit = MRLineEdit::create();
    passwordEdit->getComponent<Transform>()->setPosition(120.0f, 550.0f, 0.0f);
    passwordEdit->getComponent<Transform>()->setSize(620.0f, 58.0f);
    passwordEdit->setFontName(fontName);
    passwordEdit->setFontSize(23.0f);
    passwordEdit->setPlaceholder(L"密码输入");
    passwordEdit->setPasswordMode(true);
    passwordEdit->setMaxLength(32);
    passwordEdit->setOnSubmitCallback([](const std::wstring& text) { LOG_I("password submitted, length = {}", text.size()); });
    window->addChild(passwordEdit);

    window->addChild(createLabel(L"MRLabel 对齐与排版", fontName, 820.0f, 52.0f, 900.0f, 48.0f, 30.0f));
    window->addChild(createLabel(
        L"同一固定区域内展示水平和垂直对齐组合",
        fontName,
        820.0f,
        92.0f,
        900.0f,
        34.0f,
        18.0f));

    addAlignmentSample(
        window, fontName, L"左上 LEFT / TOP", 820.0f, 135.0f,
        HorizontalAlignment::LEFT, VerticalAlignment::TOP);
    addAlignmentSample(
        window, fontName, L"居上 CENTER / TOP", 1120.0f, 135.0f,
        HorizontalAlignment::CENTER, VerticalAlignment::TOP);
    addAlignmentSample(
        window, fontName, L"右上 RIGHT / TOP", 1420.0f, 135.0f,
        HorizontalAlignment::RIGHT, VerticalAlignment::TOP);
    addAlignmentSample(
        window, fontName, L"左下 LEFT / BOTTOM", 820.0f, 260.0f,
        HorizontalAlignment::LEFT, VerticalAlignment::BOTTOM);
    addAlignmentSample(
        window, fontName, L"完全居中", 1120.0f, 260.0f,
        HorizontalAlignment::CENTER, VerticalAlignment::CENTER);
    addAlignmentSample(
        window, fontName, L"右下 RIGHT / BOTTOM", 1420.0f, 260.0f,
        HorizontalAlignment::RIGHT, VerticalAlignment::BOTTOM);

    auto richText = MRRichTextLabel::create();
    richText->getComponent<Transform>()->setPosition(820.0f, 455.0f, 0.0f);
    richText->getComponent<Transform>()->setSize(900.0f, 300.0f);
    richText->setFontSize(30.0f);
    richText->setFontColor(0.12f, 0.16f, 0.22f, 1.0f);
    richText->setAutoWrap(true);
    richText->setLineSpacing(1.15f);
    richText->setText(
        L"[font_size=38][color=#1570EF]MRRichTextLabel[/color][/font_size][br]"
        L"支持普通文本、[color=#E5484D]混合颜色[/color]和"
        L"[font_size=42][color=#16A36A]不同字号[/color][/font_size]。[br]"
        L"标签可以[color=#7C3AED]嵌套 [font_size=36]组合[/font_size][/color]，"
        L"并根据组件宽度自动换行。",
        fontName);
    window->addChild(richText);

    LOG_I("TextDemo started: text input, rich text and label alignment");
    engine->render();
    return 0;
}
