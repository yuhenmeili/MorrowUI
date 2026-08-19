#include "MRPopup.h"

#include <algorithm>
#include <utility>

#include "MRButton.h"
#include "MRLabel.h"
#include "base/Transform.h"

namespace morrow {

std::shared_ptr<MRPopup> MRPopup::create() {
    auto popup = std::shared_ptr<MRPopup>(new MRPopup("MRPopup"));
    popup->initializePopup();
    return popup;
}

MRPopup::MRPopup(const char* widgetType) : UIWidget(false) {
    setWidgetType(widgetType);
    setDisplayLayer(10);
    m_background = MRColor::create();
    m_background->setColor(0.98f, 0.99f, 1.0f, 1.0f);
    m_background->setRounding(8.0f);
    setVisible(false);
}

void MRPopup::initializePopup() {
    addChild(m_background);
}

void MRPopup::attachTo(const std::shared_ptr<Widget>& root) {
    if (root && root.get() != this && m_parent != root)
        root->addChild(shared_from_this());
}

void MRPopup::popup(float x, float y) {
    if (!m_parent)
        return;
    getComponent<Transform>()->setPosition(x, y, 0.0f);
    setVisible(true);
    m_open = true;
    requestRender("popup");
}

void MRPopup::hide() {
    if (!m_open && !getVisible())
        return;
    closeInternal();
}

void MRPopup::closeInternal() {
    m_open = false;
    setVisible(false);
    if (m_onClosed)
        m_onClosed();
}

bool MRPopup::isOpen() const {
    return m_open;
}

void MRPopup::setOnClosedCallback(std::function<void()> callback) {
    m_onClosed = std::move(callback);
}

MRPopupPanel::MRPopupPanel() : MRPopup("MRPopupPanel") {
    m_title = std::make_shared<MRLabel>();
    m_title->setFontSize(24.0f);
    m_title->setFontColor(0.08f, 0.12f, 0.18f, 1.0f);
    m_title->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    layoutPanel();
}

std::shared_ptr<MRPopupPanel> MRPopupPanel::create() {
    auto panel = std::shared_ptr<MRPopupPanel>(new MRPopupPanel());
    panel->initializePopup();
    panel->addChild(panel->m_title);
    panel->layoutPanel();
    return panel;
}

void MRPopupPanel::setTitle(const std::wstring& title) {
    m_title->setText(title, "default");
}

void MRPopupPanel::setContent(const std::shared_ptr<Widget>& content) {
    if (m_content)
        removeChild(m_content);
    m_content = content;
    if (m_content)
        addChild(m_content);
    layoutPanel();
}

void MRPopupPanel::setPanelSize(float width, float height) {
    m_panelWidth = std::max(1.0f, width);
    m_panelHeight = std::max(1.0f, height);
    layoutPanel();
}

void MRPopupPanel::layoutPanel() {
    getComponent<Transform>()->setSize(m_panelWidth, m_panelHeight);
    m_background->getComponent<Transform>()->setPosition(0.0f, 0.0f, -0.1f);
    m_background->getComponent<Transform>()->setSize(m_panelWidth, m_panelHeight);
    m_title->getComponent<Transform>()->setPosition(20.0f, 16.0f, 0.0f);
    m_title->getComponent<Transform>()->setSize(m_panelWidth - 40.0f, 42.0f);
    if (m_content) {
        auto contentWidget = std::dynamic_pointer_cast<UIWidget>(m_content);
        if (contentWidget) {
            auto transform = contentWidget->getComponent<Transform>();
            transform->setPosition(20.0f, 70.0f, 0.0f);
            transform->setSize(m_panelWidth - 40.0f, m_panelHeight - 90.0f);
        }
    }
}

MRWindow::MRWindow() : MRPopupPanel() {
    setWidgetType("MRWindow");
    m_closeButton = MRButton::create();
    m_closeButton->setText(L"关闭", "default");
    m_closeButton->setTextFontSize(16.0f);
    m_closeButton->setTextColor(0.2f, 0.24f, 0.3f, 1.0f);
    m_closeButton->setBackgroundColor(0.88f, 0.91f, 0.95f, 1.0f);
    m_closeButton->setHoverColor(Vector4(0.78f, 0.84f, 0.92f, 1.0f));
    m_closeButton->setCornerRadius(5.0f);
    m_closeButton->setOnClickCallback([this]() { hide(); });
    m_closeButton->getComponent<Transform>()->setPosition(m_panelWidth - 92.0f, 16.0f, 0.0f);
    m_closeButton->getComponent<Transform>()->setSize(72.0f, 38.0f);
    layoutPanel();
}

std::shared_ptr<MRWindow> MRWindow::create() {
    auto window = std::shared_ptr<MRWindow>(new MRWindow());
    window->initializePopup();
    window->addChild(window->m_title);
    window->addChild(window->m_closeButton);
    window->layoutPanel();
    return window;
}

void MRWindow::setCloseButtonVisible(bool visible) {
    m_closeButton->setVisible(visible);
}

void MRWindow::setCloseButtonText(const std::wstring& text) {
    m_closeButton->setText(text, "default");
}

MRDialog::MRDialog() : MRWindow() {
    setWidgetType("MRDialog");
    setPanelSize(520.0f, 280.0f);
    m_message = std::make_shared<MRLabel>();
    m_message->setFontSize(22.0f);
    m_message->setFontColor(0.15f, 0.18f, 0.24f, 1.0f);
    m_message->setAutoWrap(true);
    m_message->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::TOP);
    m_confirmButton = MRButton::create();
    m_cancelButton = MRButton::create();
    for (const auto& button : {m_confirmButton, m_cancelButton}) {
        button->setTextFontSize(18.0f);
        button->setTextColor(0.1f, 0.14f, 0.2f, 1.0f);
        button->setBackgroundColor(0.84f, 0.9f, 0.96f, 1.0f);
        button->setHoverColor(Vector4(0.74f, 0.84f, 0.93f, 1.0f));
        button->setCornerRadius(5.0f);
    }
    m_confirmButton->setText(L"确认", "default");
    m_cancelButton->setText(L"取消", "default");
    m_confirmButton->setOnClickCallback([this]() {
        hide();
        if (m_onConfirmed) m_onConfirmed();
    });
    m_cancelButton->setOnClickCallback([this]() {
        hide();
        if (m_onCanceled) m_onCanceled();
    });
    layoutPanel();
    m_message->getComponent<Transform>()->setPosition(20.0f, 78.0f, 0.0f);
    m_message->getComponent<Transform>()->setSize(m_panelWidth - 40.0f, 100.0f);
    m_confirmButton->getComponent<Transform>()->setPosition(m_panelWidth - 220.0f, m_panelHeight - 62.0f, 0.0f);
    m_confirmButton->getComponent<Transform>()->setSize(92.0f, 42.0f);
    m_cancelButton->getComponent<Transform>()->setPosition(m_panelWidth - 116.0f, m_panelHeight - 62.0f, 0.0f);
    m_cancelButton->getComponent<Transform>()->setSize(92.0f, 42.0f);
}

std::shared_ptr<MRDialog> MRDialog::create() {
    auto dialog = std::shared_ptr<MRDialog>(new MRDialog());
    dialog->initializePopup();
    dialog->addChild(dialog->m_title);
    dialog->addChild(dialog->m_closeButton);
    dialog->addChild(dialog->m_message);
    dialog->addChild(dialog->m_confirmButton);
    dialog->addChild(dialog->m_cancelButton);
    dialog->layoutPanel();
    dialog->m_message->getComponent<Transform>()->setPosition(20.0f, 78.0f, 0.0f);
    dialog->m_message->getComponent<Transform>()->setSize(dialog->m_panelWidth - 40.0f, 100.0f);
    dialog->m_confirmButton->getComponent<Transform>()->setPosition(dialog->m_panelWidth - 220.0f, dialog->m_panelHeight - 62.0f, 0.0f);
    dialog->m_confirmButton->getComponent<Transform>()->setSize(92.0f, 42.0f);
    dialog->m_cancelButton->getComponent<Transform>()->setPosition(dialog->m_panelWidth - 116.0f, dialog->m_panelHeight - 62.0f, 0.0f);
    dialog->m_cancelButton->getComponent<Transform>()->setSize(92.0f, 42.0f);
    return dialog;
}

void MRDialog::setMessage(const std::wstring& message) {
    m_message->setText(message, "default");
    m_message->getComponent<Transform>()->setSize(m_panelWidth - 40.0f, 100.0f);
}

void MRDialog::setConfirmText(const std::wstring& text) {
    m_confirmButton->setText(text, "default");
}

void MRDialog::setCancelText(const std::wstring& text) {
    m_cancelButton->setText(text, "default");
}

void MRDialog::setOnConfirmedCallback(std::function<void()> callback) {
    m_onConfirmed = std::move(callback);
}

void MRDialog::setOnCanceledCallback(std::function<void()> callback) {
    m_onCanceled = std::move(callback);
}

MRTooltip::MRTooltip() : MRPopupPanel() {
    setWidgetType("MRTooltip");
    setPanelSize(360.0f, 100.0f);
    m_title->setVisible(false);
}

std::shared_ptr<MRTooltip> MRTooltip::create() {
    auto tooltip = std::shared_ptr<MRTooltip>(new MRTooltip());
    tooltip->initializePopup();
    tooltip->addChild(tooltip->m_title);
    tooltip->layoutPanel();
    return tooltip;
}

void MRTooltip::setText(const std::wstring& text) {
    auto label = std::make_shared<MRLabel>();
    label->setText(text, "default");
    label->setFontSize(18.0f);
    label->setFontColor(0.12f, 0.16f, 0.22f, 1.0f);
    label->setAutoWrap(true);
    label->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    setContent(label);
}

void MRTooltip::showFor(const Math::Rect& targetBounds) {
    popup(targetBounds.Min.x + m_offset.x, targetBounds.Max.y + m_offset.y);
}

void MRTooltip::setOffset(float x, float y) {
    m_offset = Vector2(x, y);
}

}  // namespace morrow
