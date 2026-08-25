#include "MRTextEdit.h"

#include <algorithm>
#include <cmath>

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
    m_characterConnection = interaction->addEventListener(TOUCH_EVENT_TYPE_CHARACTER, [this](TouchEvent& event) { handleCharacter(event.unicodeCodepoint); });
    m_keyDownConnection = interaction->addEventListener(TOUCH_EVENT_TYPE_KEY_DOWN, [this](TouchEvent& event) { handleKeyDown(event); });
    m_pointerDownConnection = interaction->addEventListener(TOUCH_EVENT_TYPE_TOUCH, [this](TouchEvent& event) { handlePointerDown(event); });
    m_pointerMoveConnection = interaction->addEventListener(TOUCH_EVENT_TYPE_MOVE, [this](TouchEvent& event) { handlePointerMove(event); });
    m_pointerReleaseConnection = interaction->addEventListener(TOUCH_EVENT_TYPE_RELEASE, [this](TouchEvent& event) { handlePointerRelease(event); });

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
    m_selectionAnchor = m_cursorPosition;
    m_selectionPosition = m_cursorPosition;
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

void MRTextEdit::setAutoWrap(bool enabled) {
    m_label->setAutoWrap(enabled);
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

MRTextEdit::Events& MRTextEdit::events() {
    return m_events;
}

void MRTextEdit::setCursorPosition(size_t position) {
    m_cursorPosition = std::min(position, m_text.size());
    m_selectionAnchor = m_cursorPosition;
    m_selectionPosition = m_cursorPosition;
    updateSelectionVisuals();
    updateCursorVisual();
}

void MRTextEdit::selectAll() {
    m_selectionAnchor = 0;
    m_selectionPosition = m_text.size();
    m_cursorPosition = m_selectionPosition;
    updateSelectionVisuals();
    updateCursorVisual();
}

void MRTextEdit::clearSelection() {
    m_selectionAnchor = m_cursorPosition;
    m_selectionPosition = m_cursorPosition;
    updateSelectionVisuals();
}

bool MRTextEdit::hasSelection() const {
    return m_selectionAnchor != m_selectionPosition;
}

std::wstring MRTextEdit::getSelectedText() const {
    const size_t start = std::min(m_selectionAnchor, m_selectionPosition);
    const size_t end = std::max(m_selectionAnchor, m_selectionPosition);
    return m_text.substr(start, end - start);
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
    notifySubmitted();
}

bool MRTextEdit::acceptsCharacter(wchar_t character) const {
    return m_multiline || (character != L'\n' && character != L'\r');
}

void MRTextEdit::refreshVisuals() {
    const bool showPlaceholder = m_text.empty() && !m_focused;
    m_label->setText(showPlaceholder ? m_placeholder : buildDisplayText(), m_fontName);
    m_label->setFontColor(showPlaceholder ? m_placeholderColor : m_textColor);
    m_background->setColor(m_focused ? m_focusedBackgroundColor : m_backgroundColor);
    updateSelectionVisuals();
    updateCursorVisual();
}

void MRTextEdit::notifyTextChanged() {
    m_events.onTextChanged.notify(*this, m_text);
}

void MRTextEdit::notifySubmitted() {
    m_events.onSubmitted.notify(*this, m_text);
}

void MRTextEdit::handleCharacter(uint32_t codepoint) {
    if (m_readOnly || codepoint < 32 || codepoint > 0x10FFFF)
        return;
    const wchar_t character = static_cast<wchar_t>(codepoint);
    if (acceptsCharacter(character))
        insertCharacter(character);
}

void MRTextEdit::handleKeyDown(const TouchEvent& event) {
    const bool control = (event.modifiers & TOUCH_MODIFIER_CTRL) != 0;
    const bool shift = (event.modifiers & TOUCH_MODIFIER_SHIFT) != 0;
    if (control && event.keyCode == TOUCH_KEY_A) {
        selectAll();
        return;
    }
    if (control && event.keyCode == TOUCH_KEY_C) {
        const auto selected = getSelectedText();
        if (!selected.empty())
            m_events.onCopyRequested.notify(*this, selected);
        return;
    }

    const auto moveCursor = [this, shift](size_t position) {
        if (!shift)
            m_selectionAnchor = position;
        m_cursorPosition = position;
        m_selectionPosition = position;
        updateSelectionVisuals();
        updateCursorVisual();
    };
    switch (event.keyCode) {
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
            moveCursor(m_cursorPosition > 0 ? m_cursorPosition - 1 : 0);
            break;
        case TOUCH_KEY_RIGHT:
            moveCursor(m_cursorPosition < m_text.size() ? m_cursorPosition + 1 : m_text.size());
            break;
        case TOUCH_KEY_HOME:
            moveCursor(0);
            break;
        case TOUCH_KEY_END:
            moveCursor(m_text.size());
            break;
        default:
            break;
    }
}

void MRTextEdit::handlePointerDown(const TouchEvent& event) {
    if (event.button != TOUCH_MOUSE_BUTTON_LEFT)
        return;
    const size_t position = textPositionAt(event.positionX, event.positionY);
    m_cursorPosition = position;
    m_selectionAnchor = position;
    m_selectionPosition = position;
    m_selectingWithPointer = true;
    updateSelectionVisuals();
    updateCursorVisual();
}

void MRTextEdit::handlePointerMove(const TouchEvent& event) {
    if (!m_selectingWithPointer || (event.buttonsMask & TOUCH_BUTTON_FLAG_LEFT) == 0) {
        return;
    }
    m_cursorPosition = textPositionAt(event.positionX, event.positionY);
    m_selectionPosition = m_cursorPosition;
    updateSelectionVisuals();
    updateCursorVisual();
}

void MRTextEdit::handlePointerRelease(const TouchEvent& event) {
    if (!m_selectingWithPointer)
        return;
    m_cursorPosition = textPositionAt(event.positionX, event.positionY);
    m_selectionPosition = m_cursorPosition;
    m_selectingWithPointer = false;
    updateSelectionVisuals();
    updateCursorVisual();
}

void MRTextEdit::insertCharacter(wchar_t character) {
    eraseSelection();
    if (m_maxLength > 0 && m_text.size() >= m_maxLength)
        return;
    m_text.insert(m_text.begin() + static_cast<std::ptrdiff_t>(m_cursorPosition), character);
    ++m_cursorPosition;
    clearSelection();
    refreshVisuals();
    notifyTextChanged();
}

void MRTextEdit::eraseBeforeCursor() {
    if (hasSelection()) {
        eraseSelection();
        refreshVisuals();
        notifyTextChanged();
        return;
    }
    if (m_cursorPosition == 0 || m_text.empty())
        return;
    m_text.erase(m_cursorPosition - 1, 1);
    --m_cursorPosition;
    clearSelection();
    refreshVisuals();
    notifyTextChanged();
}

void MRTextEdit::eraseAtCursor() {
    if (hasSelection()) {
        eraseSelection();
        refreshVisuals();
        notifyTextChanged();
        return;
    }
    if (m_cursorPosition >= m_text.size())
        return;
    m_text.erase(m_cursorPosition, 1);
    refreshVisuals();
    notifyTextChanged();
}

void MRTextEdit::eraseSelection() {
    if (!hasSelection())
        return;
    const size_t start = std::min(m_selectionAnchor, m_selectionPosition);
    const size_t end = std::max(m_selectionAnchor, m_selectionPosition);
    m_text.erase(start, end - start);
    m_cursorPosition = start;
    m_selectionAnchor = start;
    m_selectionPosition = start;
    updateSelectionVisuals();
}

size_t MRTextEdit::textPositionAt(float screenX, float screenY) const {
    const auto bounds = getScreenSpaceAABB();
    constexpr float padding = 12.0f;
    const float characterWidth = std::max(1.0f, m_fontSize * 0.58f);
    const float lineHeight = std::max(1.0f, m_fontSize * 1.2f);
    const size_t requestedLine = m_multiline ? static_cast<size_t>(std::max(0.0f, std::floor((screenY - bounds.Min.y - 6.0f) / lineHeight))) : 0;
    const size_t requestedColumn = static_cast<size_t>(std::max(0.0f, std::floor((screenX - bounds.Min.x - padding + characterWidth * 0.5f) / characterWidth)));

    size_t line = 0;
    size_t lineStart = 0;
    while (line < requestedLine && lineStart < m_text.size()) {
        const size_t newline = m_text.find(L'\n', lineStart);
        if (newline == std::wstring::npos)
            return m_text.size();
        lineStart = newline + 1;
        ++line;
    }
    const size_t newline = m_text.find(L'\n', lineStart);
    const size_t lineEnd = newline == std::wstring::npos ? m_text.size() : newline;
    return std::min(lineStart + requestedColumn, lineEnd);
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
    updateSelectionVisuals();
    updateCursorVisual();
}

void MRTextEdit::updateSelectionVisuals() {
    const size_t selectionStart = std::min(m_selectionAnchor, m_selectionPosition);
    const size_t selectionEnd = std::max(m_selectionAnchor, m_selectionPosition);
    if (selectionStart == selectionEnd) {
        for (const auto& rectangle : m_selectionRects)
            rectangle->setVisible(false);
        return;
    }

    std::vector<std::pair<size_t, size_t>> ranges;
    size_t lineStart = 0;
    while (lineStart <= m_text.size()) {
        const size_t newline = m_text.find(L'\n', lineStart);
        const size_t lineEnd = newline == std::wstring::npos ? m_text.size() : newline;
        const size_t start = std::max(selectionStart, lineStart);
        const size_t end = std::min(selectionEnd, lineEnd);
        if (start < end || (newline != std::wstring::npos && selectionStart <= newline && selectionEnd > newline)) {
            ranges.emplace_back(start, std::max(start, end));
        }
        if (newline == std::wstring::npos)
            break;
        lineStart = newline + 1;
    }

    while (m_selectionRects.size() < ranges.size()) {
        auto rectangle = MRColor::create();
        rectangle->setColor(0.18f, 0.45f, 0.78f, 0.62f);
        addChild(rectangle);
        m_selectionRects.push_back(rectangle);
    }

    const float characterWidth = m_fontSize * 0.58f;
    const float lineHeight = m_fontSize * 1.2f;
    size_t rangeIndex = 0;
    size_t currentLineStart = 0;
    for (size_t line = 0; line < ranges.size(); ++line) {
        const auto [start, end] = ranges[line];
        while (currentLineStart < start) {
            const size_t newline = m_text.find(L'\n', currentLineStart);
            if (newline == std::wstring::npos || newline >= start)
                break;
            currentLineStart = newline + 1;
            ++rangeIndex;
        }
        auto& rectangle = m_selectionRects[line];
        const float x = 12.0f + static_cast<float>(start - currentLineStart) * characterWidth;
        const float y = m_multiline ? 6.0f + static_cast<float>(rangeIndex) * lineHeight : std::max(0.0f, (getComponent<Transform>()->getSize().y - m_fontSize) * 0.5f);
        const float width = std::max(characterWidth * 0.4f, static_cast<float>(end - start) * characterWidth);
        rectangle->getComponent<Transform>()->setPosition(x, y, 0.05f);
        rectangle->getComponent<Transform>()->setSize(width, lineHeight);
        rectangle->setVisible(true);
    }
    for (size_t index = ranges.size(); index < m_selectionRects.size(); ++index) {
        m_selectionRects[index]->setVisible(false);
    }
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
