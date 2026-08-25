#include <iostream>
#include <memory>
#include <string>

#include "ui/base/TouchEvent.h"
#include "ui/elements/MRLineEdit.h"
#include "ui/elements/MRTextEdit.h"

using namespace morrow;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

void typeCharacter(const std::shared_ptr<Widget>& widget, wchar_t character) {
    TouchEvent event;
    event.eventType = TOUCH_EVENT_TYPE_CHARACTER;
    event.deviceType = TOUCH_DEVICE_TYPE_KEYBOARD;
    event.unicodeCodepoint = static_cast<uint32_t>(character);
    widget->dispatchTouchEvent(event);
}

void pressKey(
    const std::shared_ptr<Widget>& widget,
    TouchKeyCode keyCode,
    uint32_t modifiers = TOUCH_MODIFIER_NONE) {
    TouchEvent event;
    event.eventType = TOUCH_EVENT_TYPE_KEY_DOWN;
    event.deviceType = TOUCH_DEVICE_TYPE_KEYBOARD;
    event.keyCode = keyCode;
    event.modifiers = modifiers;
    widget->dispatchTouchEvent(event);
}

void testMultilineEditing() {
    auto edit = MRTextEdit::create();
    int changedCount = 0;
    bool submitted = false;
    auto changedConnection = edit->events().onTextChanged.connect(
        [&changedCount](MRTextEdit&, const std::wstring&) { ++changedCount; });
    auto submitConnection = edit->events().onSubmitted.connect(
        [&submitted](MRTextEdit&, const std::wstring&) { submitted = true; });

    typeCharacter(edit, L'A');
    typeCharacter(edit, L'中');
    pressKey(edit, TOUCH_KEY_ENTER);
    typeCharacter(edit, L'B');

    expect(edit->getText() == L"A中\nB", "multiline edit should accept characters and newline input");
    expect(submitted, "multiline enter should invoke the submit callback");
    expect(changedCount == 4, "each multiline text mutation should invoke the changed callback");

    pressKey(edit, TOUCH_KEY_LEFT);
    pressKey(edit, TOUCH_KEY_BACKSPACE);
    expect(edit->getText() == L"A中B", "backspace should remove the character before the cursor");
}

void testLineEditSubmissionAndPasswordMode() {
    auto edit = MRLineEdit::create();
    edit->setPasswordMode(true);
    edit->setMaxLength(4);

    std::wstring submittedText;
    auto submitConnection = edit->events().onSubmitted.connect(
        [&submittedText](MRTextEdit&, const std::wstring& text) {
            submittedText = text;
        });

    typeCharacter(edit, L'1');
    typeCharacter(edit, L'2');
    typeCharacter(edit, L'3');
    typeCharacter(edit, L'4');
    typeCharacter(edit, L'5');
    pressKey(edit, TOUCH_KEY_ENTER);

    expect(edit->getText() == L"1234", "line edit should preserve raw password text and enforce max length");
    expect(submittedText == L"1234", "line edit enter should submit without inserting a newline");

    edit->setMaxLength(0);
    edit->setText(L"first\nsecond");
    expect(edit->getText() == L"firstsecond", "line edit should remove newline characters from assigned text");
}

void testSelectionCopyAndReplacement() {
    auto edit = MRTextEdit::create();
    edit->setText(L"build output");
    edit->setReadOnly(true);

    std::wstring copied;
    auto copyConnection = edit->events().onCopyRequested.connect(
        [&copied](MRTextEdit&, const std::wstring& text) {
            copied = text;
        });
    pressKey(edit, TOUCH_KEY_A, TOUCH_MODIFIER_CTRL);
    expect(edit->hasSelection(), "Ctrl+A should select text in a read-only edit");
    expect(
        edit->getSelectedText() == L"build output",
        "read-only selection should expose the selected log text");
    pressKey(edit, TOUCH_KEY_C, TOUCH_MODIFIER_CTRL);
    expect(
        copied == L"build output",
        "Ctrl+C should request a copy of the selected text");

    edit->setReadOnly(false);
    typeCharacter(edit, L'X');
    expect(
        edit->getText() == L"X",
        "typing should replace the active selection");
    expect(!edit->hasSelection(), "replacement should clear the selection");
}

}  // namespace

int main() {
    testMultilineEditing();
    testLineEditSubmissionAndPasswordMode();
    testSelectionCopyAndReplacement();

    if (g_failures != 0) {
        std::cerr << g_failures << " text edit test(s) failed\n";
        return 1;
    }
    std::cout << "All text edit tests passed\n";
    return 0;
}
