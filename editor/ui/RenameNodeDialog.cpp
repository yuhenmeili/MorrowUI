#include "RenameNodeDialog.h"

#include <codecvt>
#include <locale>

#include "base/BaseButton.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "elements/MRLabel.h"
#include "elements/MRLineEdit.h"

namespace {

std::wstring wide(const std::string& text) {
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(text);
    } catch (...) {
        return std::wstring(text.begin(), text.end());
    }
}

std::string narrow(const std::wstring& text) {
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(text);
    } catch (...) {
        return std::string(text.begin(), text.end());
    }
}

std::shared_ptr<morrow::MRButton> makeBox(
    const morrow::Vector4& color,
    bool interactive) {
    auto value = morrow::MRButton::create();
    value->setText(L"", "default");
    value->setInteractive(interactive);
    value->setCornerRadius(4.0f);
    value->setBackgroundColor(color);
    value->setHoverColor(color);
    value->setPressedColor(color);
    return value;
}

}  // namespace

namespace morrow::editor {

std::shared_ptr<RenameNodeDialog> RenameNodeDialog::create() {
    auto value = std::shared_ptr<RenameNodeDialog>(
        new RenameNodeDialog());
    value->initializeControls();
    return value;
}

RenameNodeDialog::RenameNodeDialog() : UIWidget(false) {
    setWidgetType("EditorRenameNodeDialog");
    setDisplayLayer(10);
    setVisible(false);
    getTransform()->addSizeChangeListener([this]() { layoutControls(); });
}

void RenameNodeDialog::initializeControls() {
    m_backdrop = makeBox(
        Vector4(0.015f, 0.02f, 0.03f, 0.58f), true);
    addChild(m_backdrop);
    m_panel = makeBox(Vector4(0.12f, 0.13f, 0.15f, 1.0f), false);
    addChild(m_panel);

    m_title = std::make_shared<MRLabel>();
    m_title->setText(L"Rename Node", "default");
    m_title->setFontSize(19.0f);
    m_title->setFontColor(0.96f, 0.97f, 0.99f, 1.0f);
    m_panel->addChild(m_title);

    m_nameEdit = MRLineEdit::create();
    m_nameEdit->setFontSize(16.0f);
    m_nameEdit->setBackgroundColor(
        Vector4(0.07f, 0.08f, 0.10f, 1.0f));
    m_nameEdit->setFocusedBackgroundColor(
        Vector4(0.10f, 0.13f, 0.18f, 1.0f));
    m_nameEdit->setTextColor(
        Vector4(0.95f, 0.96f, 0.98f, 1.0f));
    m_submitConnection =
        m_nameEdit->events().onSubmitted.connect(
            [this](MRTextEdit&, const std::wstring&) { confirm(); });
    m_panel->addChild(m_nameEdit);

    m_confirmButton = MRButton::create();
    m_confirmButton->setText(L"Rename", "default");
    m_confirmButton->setBackgroundColor(
        Vector4(0.18f, 0.34f, 0.59f, 1.0f));
    m_confirmButton->setHoverColor(
        Vector4(0.27f, 0.49f, 0.78f, 1.0f));
    m_confirmConnection =
        m_confirmButton->events().onClicked.connect(
            [this](BaseButton&) { confirm(); });
    m_panel->addChild(m_confirmButton);

    m_cancelButton = MRButton::create();
    m_cancelButton->setText(L"Cancel", "default");
    m_cancelButton->setBackgroundColor(
        Vector4(0.24f, 0.25f, 0.28f, 1.0f));
    m_cancelButton->setHoverColor(
        Vector4(0.34f, 0.36f, 0.40f, 1.0f));
    m_cancelConnection =
        m_cancelButton->events().onClicked.connect(
            [this](BaseButton&) { hideDialog(); });
    m_panel->addChild(m_cancelButton);
}

void RenameNodeDialog::show(
    const std::string& nodeId,
    const std::string& currentName) {
    m_nodeId = nodeId;
    m_nameEdit->setText(wide(currentName));
    m_open = true;
    setVisible(true);
}

void RenameNodeDialog::hideDialog() {
    m_open = false;
    setVisible(false);
}

bool RenameNodeDialog::isOpen() const {
    return m_open;
}

RenameNodeDialog::Events& RenameNodeDialog::events() {
    return m_events;
}

void RenameNodeDialog::update(FrameStateSharedPtr frameState) {
    layoutControls();
    UIWidget::update(frameState);
}

void RenameNodeDialog::layoutControls() {
    if (!m_panel)
        return;
    const Vector3 size = getTransform()->getSize();
    m_backdrop->getTransform()->setPosition(0.0f, 0.0f, 0.0f);
    m_backdrop->getTransform()->setSize(size.x, size.y);
    constexpr float width = 480.0f;
    constexpr float height = 190.0f;
    m_panel->getTransform()->setPosition(
        (size.x - width) * 0.5f,
        (size.y - height) * 0.5f,
        1.0f);
    m_panel->getTransform()->setSize(width, height);
    m_title->getTransform()->setPosition(20.0f, 16.0f, 0.0f);
    m_title->getTransform()->setSize(width - 40.0f, 28.0f);
    m_nameEdit->getTransform()->setPosition(20.0f, 58.0f, 0.0f);
    m_nameEdit->getTransform()->setSize(width - 40.0f, 38.0f);
    m_confirmButton->getTransform()->setPosition(
        70.0f, 124.0f, 0.0f);
    m_confirmButton->getTransform()->setSize(140.0f, 38.0f);
    m_cancelButton->getTransform()->setPosition(
        270.0f, 124.0f, 0.0f);
    m_cancelButton->getTransform()->setSize(140.0f, 38.0f);
}

void RenameNodeDialog::confirm() {
    const auto& text = m_nameEdit->getText();
    const std::string name = narrow(text);
    if (name.empty())
        return;
    m_events.onConfirmed.notify(*this, m_nodeId, name);
    hideDialog();
}

}  // namespace morrow::editor
