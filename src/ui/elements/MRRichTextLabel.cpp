//
// Created by 0060328 on 25-10-11.
//

#include "MRRichTextLabel.h"

#include <algorithm>
#include <cstdint>
#include <cwctype>
#include <utility>

#include "FontManager.h"
#include "GlobalObject.h"
#include "base/Transform.h"

namespace morrow {
namespace {

std::wstring trim(const std::wstring& value) {
    size_t first = 0;
    while (first < value.size() && std::iswspace(value[first])) {
        ++first;
    }
    size_t last = value.size();
    while (last > first && std::iswspace(value[last - 1])) {
        --last;
    }
    return value.substr(first, last - first);
}

std::wstring toLowerAscii(std::wstring value) {
    for (auto& character : value) {
        if (character >= L'A' && character <= L'Z') {
            character = static_cast<wchar_t>(character - L'A' + L'a');
        }
    }
    return value;
}

int hexDigit(wchar_t character) {
    if (character >= L'0' && character <= L'9') {
        return character - L'0';
    }
    if (character >= L'a' && character <= L'f') {
        return character - L'a' + 10;
    }
    if (character >= L'A' && character <= L'F') {
        return character - L'A' + 10;
    }
    return -1;
}

bool parseHexColor(const std::wstring& value, Vector4& color) {
    std::wstring digits = trim(value);
    if (!digits.empty() && digits.front() == L'#') {
        digits.erase(digits.begin());
    }
    if (digits.size() != 6 && digits.size() != 8) {
        return false;
    }

    uint32_t packed = 0;
    for (const wchar_t character : digits) {
        const int digit = hexDigit(character);
        if (digit < 0) {
            return false;
        }
        packed = (packed << 4u) | static_cast<uint32_t>(digit);
    }

    if (digits.size() == 6) {
        packed = (packed << 8u) | 0xFFu;
    }
    color.set(
        static_cast<float>((packed >> 24u) & 0xFFu) / 255.0f,
        static_cast<float>((packed >> 16u) & 0xFFu) / 255.0f,
        static_cast<float>((packed >> 8u) & 0xFFu) / 255.0f,
        static_cast<float>(packed & 0xFFu) / 255.0f);
    return true;
}

bool parseFontSize(const std::wstring& value, float& fontSize) {
    try {
        const std::wstring normalizedValue = trim(value);
        size_t parsedCharacters = 0;
        const float parsed = std::stof(normalizedValue, &parsedCharacters);
        if (parsedCharacters != normalizedValue.size() || parsed <= 0.0f) {
            return false;
        }
        fontSize = std::max(1.0f, parsed);
        return true;
    } catch (...) {
        return false;
    }
}

}  // namespace

MRRichTextLabelSharedPtr MRRichTextLabel::create() {
    return std::shared_ptr<MRRichTextLabel>(new MRRichTextLabel());
}

MRRichTextLabel::MRRichTextLabel()
    : UIWidget(false) {
    setWidgetType("MRRichTextLabel");
    getComponent<Transform>()->addSizeChangeListener([this]() {
        m_layoutDirty = true;
        requestRender("richTextSizeChanged");
    });
}

bool MRRichTextLabel::stylesEqual(const TextStyle& left, const TextStyle& right) {
    return left.color == right.color && left.fontSize == right.fontSize;
}

void MRRichTextLabel::setText(const std::wstring& text, const std::string& fontName) {
    if (m_text == text && m_fontName == fontName) {
        return;
    }
    m_text = text;
    m_fontName = fontName;
    markTextDirty("setRichText");
}

void MRRichTextLabel::appendText(const std::wstring& text) {
    if (text.empty()) {
        return;
    }
    m_text += text;
    markTextDirty("appendRichText");
}

void MRRichTextLabel::clear() {
    if (m_text.empty() && m_plainText.empty() && m_runLabels.empty()) {
        return;
    }
    m_text.clear();
    markTextDirty("clearRichText");
}

void MRRichTextLabel::setFontName(const std::string& fontName) {
    if (m_fontName == fontName) {
        return;
    }
    m_fontName = fontName;
    m_layoutDirty = true;
    requestRender("setRichTextFontName");
}

void MRRichTextLabel::setFontSize(float fontSize) {
    fontSize = std::max(1.0f, fontSize);
    if (m_fontSize == fontSize) {
        return;
    }
    m_fontSize = fontSize;
    markTextDirty("setRichTextFontSize");
}

void MRRichTextLabel::setFontColor(const Vector4& color) {
    if (m_fontColor == color) {
        return;
    }
    m_fontColor = color;
    markTextDirty("setRichTextFontColor");
}

void MRRichTextLabel::setFontColor(float r, float g, float b, float a) {
    setFontColor(Vector4(r, g, b, a));
}

void MRRichTextLabel::setBbcodeEnabled(bool enabled) {
    if (m_bbcodeEnabled == enabled) {
        return;
    }
    m_bbcodeEnabled = enabled;
    markTextDirty("setBbcodeEnabled");
}

void MRRichTextLabel::setAutoWrap(bool enabled) {
    if (m_autoWrap == enabled) {
        return;
    }
    m_autoWrap = enabled;
    m_layoutDirty = true;
    requestRender("setRichTextAutoWrap");
}

void MRRichTextLabel::setCharacterSpacing(float spacing) {
    if (m_characterSpacing == spacing) {
        return;
    }
    m_characterSpacing = spacing;
    m_layoutDirty = true;
    requestRender("setRichTextCharacterSpacing");
}

void MRRichTextLabel::setLineSpacing(float spacing) {
    spacing = std::max(0.1f, spacing);
    if (m_lineSpacing == spacing) {
        return;
    }
    m_lineSpacing = spacing;
    m_layoutDirty = true;
    requestRender("setRichTextLineSpacing");
}

void MRRichTextLabel::setMaxLines(int maxLines) {
    maxLines = std::max(0, maxLines);
    if (m_maxLines == maxLines) {
        return;
    }
    m_maxLines = maxLines;
    m_layoutDirty = true;
    requestRender("setRichTextMaxLines");
}

void MRRichTextLabel::setAlign(HorizontalAlignment horizontal, VerticalAlignment vertical) {
    if (m_horizontalAlignment == horizontal && m_verticalAlignment == vertical) {
        return;
    }
    m_horizontalAlignment = horizontal;
    m_verticalAlignment = vertical;
    m_layoutDirty = true;
    requestRender("setRichTextAlign");
}

void MRRichTextLabel::update(FrameStateSharedPtr frameState) {
    if (m_layoutDirty) {
        parseText();
        rebuildLayout();
        m_layoutDirty = false;
    }
    UIWidget::update(frameState);
}

void MRRichTextLabel::parseText() {
    m_plainText.clear();
    m_parsedRuns.clear();

    const TextStyle defaultStyle{m_fontColor, m_fontSize};
    if (!m_bbcodeEnabled) {
        m_plainText = m_text;
        if (!m_text.empty()) {
            m_parsedRuns.push_back({m_text, defaultStyle});
        }
        return;
    }

    struct StyleEntry {
        std::wstring tag;
        TextStyle style;
    };
    std::vector<StyleEntry> styleStack{{L"", defaultStyle}};

    auto appendText = [this](const std::wstring& text, const TextStyle& style) {
        if (text.empty()) {
            return;
        }
        m_plainText += text;
        if (!m_parsedRuns.empty() && stylesEqual(m_parsedRuns.back().style, style)) {
            m_parsedRuns.back().text += text;
        } else {
            m_parsedRuns.push_back({text, style});
        }
    };

    size_t cursor = 0;
    while (cursor < m_text.size()) {
        if (m_text[cursor] != L'[') {
            const size_t nextTag = m_text.find(L'[', cursor);
            const size_t length = nextTag == std::wstring::npos ? std::wstring::npos : nextTag - cursor;
            appendText(m_text.substr(cursor, length), styleStack.back().style);
            if (nextTag == std::wstring::npos) {
                break;
            }
            cursor = nextTag;
            continue;
        }

        const size_t closeBracket = m_text.find(L']', cursor + 1);
        if (closeBracket == std::wstring::npos) {
            appendText(m_text.substr(cursor), styleStack.back().style);
            break;
        }

        const std::wstring originalTag = m_text.substr(cursor, closeBracket - cursor + 1);
        const std::wstring token = trim(m_text.substr(cursor + 1, closeBracket - cursor - 1));
        const std::wstring lowerToken = toLowerAscii(token);
        bool recognized = false;

        if (lowerToken == L"br" || lowerToken == L"br/") {
            appendText(L"\n", styleStack.back().style);
            recognized = true;
        } else if (lowerToken == L"lb") {
            appendText(L"[", styleStack.back().style);
            recognized = true;
        } else if (lowerToken == L"rb") {
            appendText(L"]", styleStack.back().style);
            recognized = true;
        } else if (lowerToken.rfind(L"color=", 0) == 0) {
            TextStyle style = styleStack.back().style;
            if (parseHexColor(token.substr(token.find(L'=') + 1), style.color)) {
                styleStack.push_back({L"color", style});
                recognized = true;
            }
        } else if (lowerToken.rfind(L"font_size=", 0) == 0 ||
                   lowerToken.rfind(L"size=", 0) == 0) {
            TextStyle style = styleStack.back().style;
            if (parseFontSize(token.substr(token.find(L'=') + 1), style.fontSize)) {
                styleStack.push_back({L"font_size", style});
                recognized = true;
            }
        } else if (lowerToken == L"/color") {
            if (styleStack.size() > 1 && styleStack.back().tag == L"color") {
                styleStack.pop_back();
                recognized = true;
            }
        } else if (lowerToken == L"/font_size" || lowerToken == L"/size") {
            if (styleStack.size() > 1 && styleStack.back().tag == L"font_size") {
                styleStack.pop_back();
                recognized = true;
            }
        }

        if (!recognized) {
            appendText(originalTag, styleStack.back().style);
        }
        cursor = closeBracket + 1;
    }
}

void MRRichTextLabel::rebuildLayout() {
    clearRunLabels();
    m_textWidth = 0.0f;
    m_textHeight = 0.0f;

    if (m_parsedRuns.empty()) {
        return;
    }

    auto font = GlobalObject::getInstance().getFontManager()->getFont(m_fontName);
    if (!font) {
        LOG_E("MRRichTextLabel failed to resolve font '{}' for text length {}",
              m_fontName, m_plainText.size());
        return;
    }

    const Vector3 containerSize = getComponent<Transform>()->getSize();
    const float wrapWidth = containerSize.x;
    std::vector<LayoutLine> lines;
    LayoutLine currentLine;
    bool currentLineHasText = false;
    bool reachedLineLimit = false;

    auto finishLine = [&](bool forceEmptyLine) {
        if (!currentLineHasText && !forceEmptyLine) {
            return true;
        }
        if (!currentLineHasText) {
            const auto& metrics = font->GetMetrics(m_fontSize);
            currentLine.ascent = metrics.ascent;
            currentLine.belowBaseline = std::max(0.0f, metrics.lineHeight - metrics.ascent);
        } else {
            currentLine.width = std::max(0.0f, currentLine.width - m_characterSpacing);
            if (!currentLine.runs.empty()) {
                currentLine.runs.back().width =
                    std::max(0.0f, currentLine.runs.back().width - m_characterSpacing);
            }
        }
        currentLine.height =
            std::max(1.0f, currentLine.ascent + currentLine.belowBaseline) * m_lineSpacing;
        lines.push_back(std::move(currentLine));
        currentLine = LayoutLine{};
        currentLineHasText = false;
        return m_maxLines <= 0 || static_cast<int>(lines.size()) < m_maxLines;
    };

    for (const auto& parsedRun : m_parsedRuns) {
        const auto& metrics = font->GetMetrics(parsedRun.style.fontSize);
        const FontGlyph* spaceGlyph = font->GetGlyph(L' ', parsedRun.style.fontSize);
        const float fallbackAdvance =
            spaceGlyph ? spaceGlyph->advance + m_characterSpacing : parsedRun.style.fontSize * 0.5f;

        for (const wchar_t character : parsedRun.text) {
            if (character == L'\n') {
                if (!finishLine(true)) {
                    reachedLineLimit = true;
                    break;
                }
                continue;
            }

            const FontGlyph* glyph = font->GetGlyph(character, parsedRun.style.fontSize);
            const float advance =
                glyph && glyph->generated ? glyph->advance + m_characterSpacing : fallbackAdvance;

            if (m_autoWrap && wrapWidth > 0.0f && currentLineHasText &&
                currentLine.width + advance > wrapWidth) {
                if (!finishLine(false)) {
                    reachedLineLimit = true;
                    break;
                }
            }

            if (currentLine.runs.empty() ||
                !stylesEqual(currentLine.runs.back().style, parsedRun.style)) {
                LayoutRun layoutRun;
                layoutRun.style = parsedRun.style;
                layoutRun.ascent = metrics.ascent;
                layoutRun.lineHeight = metrics.lineHeight;
                currentLine.runs.push_back(std::move(layoutRun));
            }
            auto& layoutRun = currentLine.runs.back();
            layoutRun.text.push_back(character);
            layoutRun.width += advance;
            currentLine.width += advance;
            currentLine.ascent = std::max(currentLine.ascent, metrics.ascent);
            currentLine.belowBaseline =
                std::max(currentLine.belowBaseline,
                         std::max(0.0f, metrics.lineHeight - metrics.ascent));
            currentLineHasText = true;
        }
        if (reachedLineLimit) {
            break;
        }
    }

    if (!reachedLineLimit) {
        finishLine(false);
    }
    font->FlushPendingUploads();

    for (const auto& line : lines) {
        m_textWidth = std::max(m_textWidth, line.width);
        m_textHeight += line.height;
    }

    float verticalOffset = 0.0f;
    switch (m_verticalAlignment) {
        case VerticalAlignment::TOP:
            break;
        case VerticalAlignment::CENTER:
            verticalOffset = (containerSize.y - m_textHeight) * 0.5f;
            break;
        case VerticalAlignment::BOTTOM:
            verticalOffset = containerSize.y - m_textHeight;
            break;
        default:
            break;
    }

    float lineY = verticalOffset;
    for (const auto& line : lines) {
        float horizontalOffset = 0.0f;
        switch (m_horizontalAlignment) {
            case HorizontalAlignment::LEFT:
            case HorizontalAlignment::JUSTIFY:
                break;
            case HorizontalAlignment::CENTER:
                horizontalOffset = (containerSize.x - line.width) * 0.5f;
                break;
            case HorizontalAlignment::RIGHT:
                horizontalOffset = containerSize.x - line.width;
                break;
        }

        float runX = horizontalOffset;
        for (const auto& run : line.runs) {
            auto label = std::make_shared<MRLabel>();
            label->setText(run.text, m_fontName);
            label->setFontSize(run.style.fontSize);
            label->setFontColor(run.style.color);
            label->setCharacterSpacing(m_characterSpacing);
            label->setLineSpacing(1.0f);
            label->setMaxLines(1);
            label->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::TOP);

            auto transform = label->getComponent<Transform>();
            transform->setPosition(
                runX,
                lineY + line.ascent - run.ascent,
                0.0f);
            transform->setSize(std::max(1.0f, run.width), std::max(1.0f, run.lineHeight));
            addChild(label);
            m_runLabels.push_back(std::move(label));
            runX += run.width;
        }
        lineY += line.height;
    }
}

void MRRichTextLabel::clearRunLabels() {
    for (const auto& label : m_runLabels) {
        removeChild(label);
    }
    m_runLabels.clear();
}

void MRRichTextLabel::markTextDirty(const char* caller) {
    parseText();
    m_layoutDirty = true;
    requestRender(caller);
}

} // morrow
