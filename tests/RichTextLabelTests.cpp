#include <iostream>
#include <string>

#include "ui/elements/MRRichTextLabel.h"

using namespace morrow;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

void testSupportedTagsProducePlainText() {
    auto label = MRRichTextLabel::create();
    label->setText(
        L"状态：[color=#16A36A]正常[/color][br]"
        L"[font_size=42]28°C[/font_size] [lb]AUTO[rb]");

    expect(label->getPlainText() == L"状态：正常\n28°C [AUTO]",
           "supported rich-text tags should be removed from plain text");
}

void testNestedTagsAndAppend() {
    auto label = MRRichTextLabel::create();
    label->setText(
        L"[color=#1570EF]蓝色 [size=40]大字[/size] 蓝色[/color]");
    label->appendText(L"[br]完成");

    expect(label->getPlainText() == L"蓝色 大字 蓝色\n完成",
           "nested tags and appended rich text should preserve visible content");
}

void testUnknownAndMalformedTagsRemainVisible() {
    auto label = MRRichTextLabel::create();
    label->setText(L"[b]粗体[/b] [color=invalid]原文[/color]");

    expect(label->getPlainText() ==
               L"[b]粗体[/b] [color=invalid]原文[/color]",
           "unsupported or malformed tags should remain visible");
}

void testBbcodeCanBeDisabled() {
    auto label = MRRichTextLabel::create();
    const std::wstring text = L"[color=#FF0000]普通文本[/color]";
    label->setBbcodeEnabled(false);
    label->setText(text);

    expect(label->getPlainText() == text,
           "disabled BBCode parsing should preserve the original text");
}

}  // namespace

int main() {
    testSupportedTagsProducePlainText();
    testNestedTagsAndAppend();
    testUnknownAndMalformedTagsRemainVisible();
    testBbcodeCanBeDisabled();

    if (g_failures != 0) {
        std::cerr << g_failures << " rich text label test(s) failed\n";
        return 1;
    }
    std::cout << "All rich text label tests passed\n";
    return 0;
}
