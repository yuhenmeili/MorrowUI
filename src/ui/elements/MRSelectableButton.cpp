#include "MRSelectableButton.h"

namespace morrow {

MRSelectableButton::MRSelectableButton() {
    setWidgetType("MRSelectableButton");
}

void MRSelectableButton::setChecked(bool checked) {
    if (checked == m_checked)
        return;
    m_checked = checked;
    updateVisualState();
    requestRender("checked state");
    onCheckedChanged(m_checked);
}

void MRSelectableButton::setOnCheckedChangedCallback(CheckedCallback callback) {
    m_onCheckedChanged = std::move(callback);
}

void MRSelectableButton::setCheckedColor(const Vector4& color) {
    m_checkedColor = color;
    updateVisualState();
}

void MRSelectableButton::setCheckedColor(float r, float g, float b, float a) {
    setCheckedColor(Vector4(r, g, b, a));
}

void MRSelectableButton::setCheckedHoverColor(const Vector4& color) {
    m_checkedHoverColor = color;
    updateVisualState();
}

void MRSelectableButton::setCheckedPressedColor(const Vector4& color) {
    m_checkedPressedColor = color;
    updateVisualState();
}

void MRSelectableButton::onActivated() {
    if (!m_checked || canUncheck()) {
        setChecked(!m_checked);
    }
}

void MRSelectableButton::updateVisualState() {
    if (!m_checked) {
        MRButton::updateVisualState();
        return;
    }

    switch (m_currentState) {
        case ButtonState::NORMAL:
        case ButtonState::FOCUSED:
            m_currentBackgroundColor = m_checkedColor;
            break;
        case ButtonState::HOVER:
            m_currentBackgroundColor = m_checkedHoverColor;
            break;
        case ButtonState::PRESSED:
            m_currentBackgroundColor = m_checkedPressedColor;
            break;
        case ButtonState::DISABLED:
            m_currentBackgroundColor = m_checkedColor;
            m_currentBackgroundColor.w *= 0.5f;
            break;
    }
}

void MRSelectableButton::onCheckedChanged(bool checked) {
    if (m_onCheckedChanged)
        m_onCheckedChanged(checked);
}

}  // namespace morrow
