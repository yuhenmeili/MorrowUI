#include "MRTextEdit.h"

#include <algorithm>

#include "base/Interaction.h"
#include "base/TouchEvent.h"
#include "base/Transform.h"

namespace morrow {

std::shared_ptr<MRTextEdit> MRTextEdit::create() {
    auto textEdit = std::shared_ptr<MRTextEdit>(new MRTextEdit(true));
    textEdit->initializeChildren();
    return textEdit;
}

MRTextEdit::MRTextEdit(bool multiline) : UIWidget(false), m_multiline(multiline) {
    setWidgetType(multiline ? "MRTextEdit" : "MRLineEdit");
    m_background = MRColor::create();
    m_label = std::make_shared<MRLabel>();
    m_cursor = MRColor::create();
    getComponent<Transform>()->addSizeChangeListener([this]() { layoutChildren(); });
}

void MRTextEdit::initializeChildren() {
    m_background->setRounding(6.0f);
    m_cursor->setColor(0.12f, 0.42f, 0.72f, 1.0f);
    m_label->setFontSize(m_fontSize);
    m_label->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::TOP);
    m_label->setAutoWrap(m_multiline);

    addChild(m_background);
    addChild(m_label);
    addChild(m_cursor);

    auto interaction = addComponent<Interaction>();
    interaction->setKeyboardFocusable(true);
    interaction->setClickEnabled(false);
    interaction->setLongPressEnabled(false);
    interaction->addEventListener(TOUCH_EVENT_TYPE_CHARACTER, [this](TouchEvent& event) { handleCharacter(event.unicodeCodepoint); });
    interaction->addEventListener(TOUCH_EVENT_TYPE_KEY_DOWN, [this](TouchEvent& event) { handleKeyDown(event.keyCode); });

    layoutChildren();
    refreshVisuals();
}

void MRTextEdit::setText(const std::wstring& text) {
    std::wstring sanitized = text;
    if (!m_multiline) {
        sanitized.erase(std::remove_if(sanitized.begin(), sanitized.end(), [](wchar_t character) { return character == L'\n' || character == L'\r'; }), sanitized.end());
    }
    if (m_maxLength > 0 && sanitized.size() > m_maxLength) {
        sanitized.resize(m_maxLength);
    }
    if (m_text == sanitized) {
        m_cursorPosition = std::min(m_cursorPosition, m_text.size());
        refreshVisuals();
        return;
    }
    m_text = std::move(sanitized);
    m_cursorPosition = m_text.size();
    refreshVisuals();
    notifyTextChanged();
}

void MRTextEdit::setPlaceholder(const std::wstring& placeholder) {
    m_placeholder = placeholder;
    refreshVisuals();
}

void MRTextEdit::setFontName(const std::string& fontName) {
    m_fontName = fontName;
    refreshVisuals();
}

void MRTextEdit::setFontSize(float fontSize) {
    m_fontSize = std::max(1.0f, fontSize);
    m_label->setFontSize(m_fontSize);
    updateCursorVisual();
}

void MRTextEdit::setTextColor(const Vector4& color) {
    m_textColor = color;
    refreshVisuals();
}

void MRTextEdit::setPlaceholderColor(const Vector4& color) {
    m_placeholderColor = color;
    refreshVisuals();
}

void MRTextEdit::setBackgroundColor(const Vector4& color) {
    m_backgroundColor = color;
    refreshVisuals();
}

void MRTextEdit::setFocusedBackgroundColor(const Vector4& color) {
    m_focusedBackgroundColor = color;
    refreshVisuals();
}

void MRTextEdit::setMaxLength(size_t maxLength) {
    m_maxLength = maxLength;
    if (m_maxLength > 0 && m_text.size() > m_maxLength) {
        setText(m_text.substr(0, m_maxLength));
    }
}

void MRTextEdit::setReadOnly(bool readOnly) {
    m_readOnly = readOnly;
    updateCursorVisual();
}

void MRTextEdit::setOnTextChangedCallback(TextChangedCallback callback) {
    m_onTextChanged = std::move(callback);
}

void MRTextEdit::setOnSubmitCallback(SubmitCallback callback) {
    m_onSubmit = std::move(callback);
}

void MRTextEdit::setCursorPosition(size_t position) {
    m_cursorPosition = std::min(position, m_text.size());
    updateCursorVisual();
}

void MRTextEdit::onFocusChanged(bool focused) {
    m_focused = focused;
    refreshVisuals();
}

std::wstring MRTextEdit::buildDisplayText() const {
    return m_text;
}

void MRTextEdit::handleEnter() {
    if (!m_readOnly)
        insertCharacter(L'\n');
    if (m_onSubmit)
        m_onSubmit(m_text);
}

bool MRTextEdit::acceptsCharacter(wchar_t character) const {
    return m_multiline || (character != L'\n' && character != L'\r');
}

void MRTextEdit::refreshVisuals() {
    const bool showPlaceholder = m_text.empty() && !m_focused;
    m_label->setText(showPlaceholder ? m_placeholder : buildDisplayText(), m_fontName);
    m_label->setFontColor(showPlaceholder ? m_placeholderColor : m_textColor);
    m_background->setColor(m_focused ? m_focusedBackgroundColor : m_backgroundColor);
    updateCursorVisual();
}

void MRTextEdit::notifyTextChanged() {
    if (m_onTextChanged)
        m_onTextChanged(m_text);
}

void MRTextEdit::handleCharacter(uint32_t codepoint) {
    if (m_readOnly || codepoint < 32 || codepoint > 0x10FFFF)
        return;
    const wchar_t character = static_cast<wchar_t>(codepoint);
    if (acceptsCharacter(character))
        insertCharacter(character);
}

void MRTextEdit::handleKeyDown(TouchKeyCode keyCode) {
    switch (keyCode) {
        case TOUCH_KEY_BACKSPACE:
            if (!m_readOnly)
                eraseBeforeCursor();
            break;
        case TOUCH_KEY_DELETE:
            if (!m_readOnly)
                eraseAtCursor();
            break;
        case TOUCH_KEY_ENTER:
            handleEnter();
            break;
        case TOUCH_KEY_LEFT:
            if (m_cursorPosition > 0)
                --m_cursorPosition;
            updateCursorVisual();
            break;
        case TOUCH_KEY_RIGHT:
            if (m_cursorPosition < m_text.size())
                ++m_cursorPosition;
            updateCursorVisual();
            break;
        case TOUCH_KEY_HOME:
            m_cursorPosition = 0;
            updateCursorVisual();
            break;
        case TOUCH_KEY_END:
            m_cursorPosition = m_text.size();
            updateCursorVisual();
            break;
        default:
            break;
    }
}

void MRTextEdit::insertCharacter(wchar_t character) {
    if (m_maxLength > 0 && m_text.size() >= m_maxLength)
        return;
    m_text.insert(m_text.begin() + static_cast<std::ptrdiff_t>(m_cursorPosition), character);
    ++m_cursorPosition;
    refreshVisuals();
    notifyTextChanged();
}

void MRTextEdit::eraseBeforeCursor() {
    if (m_cursorPosition == 0 || m_text.empty())
        return;
    m_text.erase(m_cursorPosition - 1, 1);
    --m_cursorPosition;
    refreshVisuals();
    notifyTextChanged();
}

void MRTextEdit::eraseAtCursor() {
    if (m_cursorPosition >= m_text.size())
        return;
    m_text.erase(m_cursorPosition, 1);
    refreshVisuals();
    notifyTextChanged();
}

void MRTextEdit::layoutChildren() {
    if (!m_background || !m_label || !m_cursor)
        return;
    const Vector3 size = getComponent<Transform>()->getSize();
    constexpr float padding = 12.0f;
    m_background->getComponent<Transform>()->setPosition(0.0f, 0.0f, 0.0f);
    m_background->getComponent<Transform>()->setSize(size.x, size.y);
    m_label->getComponent<Transform>()->setPosition(padding, 6.0f, 0.1f);
    m_label->getComponent<Transform>()->setSize(std::max(0.0f, size.x - padding * 2.0f), std::max(0.0f, size.y - 12.0f));
    updateCursorVisual();
}

void MRTextEdit::updateCursorVisual() {
    if (!m_cursor)
        return;
    const bool visible = m_focused && !m_readOnly;
    m_cursor->setVisible(visible);
    if (!visible)
        return;

    size_t lineStart = 0;
    size_t lineIndex = 0;
    for (size_t index = 0; index < m_cursorPosition; ++index) {
        if (m_text[index] == L'\n') {
            lineStart = index + 1;
            ++lineIndex;
        }
    }
    const size_t column = m_cursorPosition - lineStart;
    const Vector3 size = getComponent<Transform>()->getSize();
    const float estimatedCharacterWidth = m_fontSize * 0.58f;
    const float lineHeight = m_fontSize * 1.2f;
    const float cursorX = std::clamp(12.0f + static_cast<float>(column) * estimatedCharacterWidth, 12.0f, std::max(12.0f, size.x - 14.0f));
    const float cursorY = std::clamp(8.0f + static_cast<float>(lineIndex) * lineHeight, 8.0f, std::max(8.0f, size.y - lineHeight));
    m_cursor->getComponent<Transform>()->setPosition(cursorX, cursorY, 0.2f);
    m_cursor->getComponent<Transform>()->setSize(2.0f, m_fontSize);
}

}  // namespace morrow
