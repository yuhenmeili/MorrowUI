#include "Engine.h"
#include "FontManager.h"
#include "GlobalObject.h"
#include "ToolUtils.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "elements/MRImage.h"
#include "elements/MRLabel.h"
#include "elements/MRLineEdit.h"
#include "elements/MRRichTextLabel.h"
#include "elements/MRTextEdit.h"
//
// Created by 0060328 on 25-10-9.
//
using namespace morrow;

int main() {
    EngineOptions engineOptions;
    // engineOptions.multithread = false;
    EngineSharedPtr engine = std::make_shared<Engine>(engineOptions);

    auto window = engine->getWindow();
    window->setClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    FontInfo fontInfo = {.name = "debug_morrow_20", .path = "assets/fonts/MorrowSansCN1.1-Regular.otf"};
    engine->addFonts({fontInfo});

    const std::string fontName = fontInfo.name;

    auto inputTitle = std::make_shared<MRLabel>();
    inputTitle->getComponent<Transform>()->setPosition(120.0f, 90.0f, 0.0f);
    inputTitle->getComponent<Transform>()->setSize(620.0f, 48.0f);
    inputTitle->setText(L"文本输入组件", fontName);
    inputTitle->setFontSize(30.0f);
    inputTitle->setFontColor(0.12f, 0.16f, 0.22f, 1.0f);
    window->addChild(inputTitle);

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

    const std::wstring text = L"Hello World 测试";
    auto textRenderer = std::make_shared<MRLabel>();
    auto textTransform = textRenderer->getComponent<Transform>();
    textTransform->setPosition(Vector3(880.0f, 220.0f, 0.0f));
    textTransform->setSize(Vector3(700.0f, 240.0f, 0.0f));
    textRenderer->setText(text, fontName);
    textRenderer->setFontSize(35.0f);
    textRenderer->setFontColor(1.0f, 0.0f, 0.0f, 1.0f);
    window->addChild(textRenderer);

    auto richText = MRRichTextLabel::create();
    richText->getComponent<Transform>()->setPosition(820.0f, 520.0f, 0.0f);
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

    engine->render();
    return 0;
}
