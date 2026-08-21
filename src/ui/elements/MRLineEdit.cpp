#include "MRLineEdit.h"

namespace morrow {

std::shared_ptr<MRLineEdit> MRLineEdit::create() {
    auto lineEdit = std::shared_ptr<MRLineEdit>(new MRLineEdit());
    lineEdit->initializeChildren();
    lineEdit->m_label->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    lineEdit->m_label->setAutoWrap(false);
    return lineEdit;
}

MRLineEdit::MRLineEdit() : MRTextEdit(false) {
}

void MRLineEdit::setPasswordMode(bool enabled) {
    m_passwordMode = enabled;
    refreshVisuals();
}

void MRLineEdit::setPasswordCharacter(wchar_t character) {
    m_passwordCharacter = character;
    refreshVisuals();
}

std::wstring MRLineEdit::buildDisplayText() const {
    if (!m_passwordMode)
        return m_text;
    return std::wstring(m_text.size(), m_passwordCharacter);
}

void MRLineEdit::handleEnter() {
    notifySubmitted();
}

bool MRLineEdit::acceptsCharacter(wchar_t character) const {
    return character != L'\n' && character != L'\r';
}

}  // namespace morrow
