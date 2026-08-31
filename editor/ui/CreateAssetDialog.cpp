#include "CreateAssetDialog.h"

#include "base/BaseButton.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "elements/MRLabel.h"
#include "elements/MRLineEdit.h"
#include "elements/MRTree.h"

namespace morrow::editor {

CreateAssetDialog::CreateAssetDialog() : UIWidget(false) {
    setWidgetType("EditorCreateAssetDialog");
    setWidgetName("CreateAssetDialog");
    setDisplayLayer(10);
    setVisible(false);
}

std::shared_ptr<CreateAssetDialog> CreateAssetDialog::create() {
    auto dialog = std::shared_ptr<CreateAssetDialog>(new CreateAssetDialog());
    dialog->initializeControls();
    return dialog;
}

void CreateAssetDialog::initializeControls() {
    auto panelButton = MRButton::create();
    panelButton->setInteractive(false);
    panelButton->setBackgroundColor(Vector4(0.12f, 0.13f, 0.15f, 1.0f));
    m_panel = panelButton;
    addChild(m_panel);
    auto title = std::make_shared<MRLabel>();
    title->setText(L"Create New Resource", "default");
    title->setFontSize(20.0f);
    title->getTransform()->setPosition(20.0f, 16.0f, 0.0f);
    title->getTransform()->setSize(400.0f, 30.0f);
    m_panel->addChild(title);

    m_types = MRTree::create();
    m_types->setRowHeight(34.0f);
    int id = 1;
    for (const auto& type : assetTypes()) {
        m_types->addNode(id, std::wstring(type.displayName.begin(), type.displayName.end()) + L"  [" + std::wstring(type.extension.begin(), type.extension.end()) + L"]", -1, false,
                         true);
        m_typeIds[id++] = id - 1;
    }
    m_typeConnection = m_types->events().onNodeSelected.connect([this](MRTree&, int selected, const std::wstring&) {
        const auto iterator = m_typeIds.find(selected);
        if (iterator != m_typeIds.end())
            m_selectedType = assetTypes()[static_cast<size_t>(iterator->second - 1)].type;
    });
    m_types->getTransform()->setPosition(20.0f, 58.0f, 0.0f);
    m_types->getTransform()->setSize(360.0f, 180.0f);
    m_panel->addChild(m_types);

    m_name = MRLineEdit::create();
    m_name->setPlaceholder(L"Resource name");
    m_name->getTransform()->setPosition(400.0f, 58.0f, 0.0f);
    m_name->getTransform()->setSize(280.0f, 38.0f);
    m_panel->addChild(m_name);

    m_create = MRButton::create();
    m_create->setText(L"Create", "default");
    m_create->getTransform()->setPosition(400.0f, 120.0f, 0.0f);
    m_create->getTransform()->setSize(130.0f, 36.0f);
    m_createConnection = m_create->events().onClicked.connect([this](BaseButton&) { confirm(); });
    m_panel->addChild(m_create);
    m_cancel = MRButton::create();
    m_cancel->setText(L"Cancel", "default");
    m_cancel->getTransform()->setPosition(550.0f, 120.0f, 0.0f);
    m_cancel->getTransform()->setSize(130.0f, 36.0f);
    m_cancelConnection = m_cancel->events().onClicked.connect([this](BaseButton&) {
        hideDialog();
        m_events.onCanceled.notify(*this);
    });
    m_panel->addChild(m_cancel);
}

void CreateAssetDialog::show() {
    m_open = true;
    setVisible(true);
}

void CreateAssetDialog::hideDialog() {
    m_open = false;
    setVisible(false);
}

bool CreateAssetDialog::isOpen() const {
    return m_open;
}

CreateAssetDialog::Events& CreateAssetDialog::events() {
    return m_events;
}

void CreateAssetDialog::confirm() {
    std::string name(m_name->getText().begin(), m_name->getText().end());
    if (name.empty())
        return;
    for (const auto& type : assetTypes()) {
        if (type.type == m_selectedType) {
            m_events.onConfirmed.notify(*this, type, name);
            hideDialog();
            return;
        }
    }
}

void CreateAssetDialog::update(FrameStateSharedPtr state) {
    const auto size = getTransform()->getSize();
    m_panel->getTransform()->setPosition((size.x - 720.0f) * 0.5f, (size.y - 280.0f) * 0.5f, 1.0f);
    m_panel->getTransform()->setSize(720.0f, 280.0f);
    UIWidget::update(state);
}

}  // namespace morrow::editor
