#include "MRSpinBox.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "base/Transform.h"

namespace morrow {

std::shared_ptr<MRSpinBox> MRSpinBox::create() {
    auto spinBox = std::shared_ptr<MRSpinBox>(new MRSpinBox());
    spinBox->initializeChildren();
    return spinBox;
}

MRSpinBox::MRSpinBox() : UIWidget(false) {
    setWidgetType("MRSpinBox");
    m_background = MRColor::create();
    m_decreaseButton = MRButton::create();
    m_increaseButton = MRButton::create();
    m_valueLabel = std::make_shared<MRLabel>();
    getComponent<Transform>()->addSizeChangeListener([this]() { layoutChildren(); });
}

void MRSpinBox::initializeChildren() {
    m_background->setColor(0.92f, 0.94f, 0.97f, 1.0f);
    m_background->setRounding(6.0f);

    auto configureStepButton = [](const std::shared_ptr<MRButton>& button, const std::wstring& text) {
        button->setText(text, "default");
        button->setTextFontSize(24.0f);
        button->setTextColor(0.08f, 0.12f, 0.17f, 1.0f);
        button->setBackgroundColor(0.8f, 0.86f, 0.92f, 1.0f);
        button->setHoverColor(Vector4(0.7f, 0.8f, 0.9f, 1.0f));
        button->setPressedColor(Vector4(0.58f, 0.72f, 0.85f, 1.0f));
        button->setCornerRadius(5.0f);
    };
    configureStepButton(m_decreaseButton, L"-");
    configureStepButton(m_increaseButton, L"+");
    m_decreaseButton->setOnClickCallback([this]() { stepBy(-m_step); });
    m_increaseButton->setOnClickCallback([this]() { stepBy(m_step); });

    m_valueLabel->setFontSize(22.0f);
    m_valueLabel->setFontColor(0.08f, 0.12f, 0.17f, 1.0f);
    m_valueLabel->setAlign(HorizontalAlignment::CENTER, VerticalAlignment::CENTER);

    addChild(m_background);
    addChild(m_decreaseButton);
    addChild(m_valueLabel);
    addChild(m_increaseButton);
    layoutChildren();
    refreshText();
}

void MRSpinBox::setRange(double minimum, double maximum) {
    if (minimum > maximum)
        std::swap(minimum, maximum);
    m_minimum = minimum;
    m_maximum = maximum;
    setValue(m_value);
}

void MRSpinBox::setStep(double step) {
    m_step = std::max(0.000001, step);
}

void MRSpinBox::setValue(double value) {
    const double clamped = std::clamp(value, m_minimum, m_maximum);
    if (m_value == clamped) {
        refreshText();
        return;
    }
    m_value = clamped;
    refreshText();
    if (m_onValueChanged)
        m_onValueChanged(m_value);
}

void MRSpinBox::setDecimals(int decimals) {
    m_decimals = std::clamp(decimals, 0, 6);
    refreshText();
}

void MRSpinBox::setPrefix(const std::wstring& prefix) {
    m_prefix = prefix;
    refreshText();
}

void MRSpinBox::setSuffix(const std::wstring& suffix) {
    m_suffix = suffix;
    refreshText();
}

void MRSpinBox::setEnabled(bool enabled) {
    m_decreaseButton->setEnabled(enabled);
    m_increaseButton->setEnabled(enabled);
}

void MRSpinBox::setOnValueChangedCallback(ValueChangedCallback callback) {
    m_onValueChanged = std::move(callback);
}

void MRSpinBox::layoutChildren() {
    if (!m_background || !m_decreaseButton || !m_increaseButton || !m_valueLabel) {
        return;
    }
    const Vector3 size = getComponent<Transform>()->getSize();
    const float buttonWidth = std::min(52.0f, size.x * 0.25f);
    m_background->getComponent<Transform>()->setPosition(0.0f, 0.0f, 0.0f);
    m_background->getComponent<Transform>()->setSize(size.x, size.y);
    m_decreaseButton->getComponent<Transform>()->setPosition(4.0f, 4.0f, 0.1f);
    m_decreaseButton->getComponent<Transform>()->setSize(std::max(0.0f, buttonWidth - 4.0f), std::max(0.0f, size.y - 8.0f));
    m_increaseButton->getComponent<Transform>()->setPosition(size.x - buttonWidth, 4.0f, 0.1f);
    m_increaseButton->getComponent<Transform>()->setSize(std::max(0.0f, buttonWidth - 4.0f), std::max(0.0f, size.y - 8.0f));
    m_valueLabel->getComponent<Transform>()->setPosition(buttonWidth, 0.0f, 0.1f);
    m_valueLabel->getComponent<Transform>()->setSize(std::max(0.0f, size.x - buttonWidth * 2.0f), size.y);
}

void MRSpinBox::refreshText() {
    if (!m_valueLabel)
        return;
    std::wostringstream stream;
    stream << m_prefix << std::fixed << std::setprecision(m_decimals) << m_value << m_suffix;
    m_valueLabel->setText(stream.str(), "default");
}

void MRSpinBox::stepBy(double amount) {
    setValue(m_value + amount);
}

}  // namespace morrow
