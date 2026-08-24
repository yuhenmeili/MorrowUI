#include "CreateNodeDialog.h"

#include <algorithm>
#include <codecvt>
#include <locale>
#include <map>

#include "base/BaseButton.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "elements/MRLabel.h"
#include "elements/MRLineEdit.h"
#include "elements/MRTree.h"

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

std::shared_ptr<morrow::MRButton> panel(const morrow::Vector4& color, bool interactive = false) {
    auto value = morrow::MRButton::create();
    value->setText(L"", "default");
    value->setInteractive(interactive);
    value->setCornerRadius(4.0f);
    value->setBackgroundColor(color);
    return value;
}

std::shared_ptr<morrow::MRLabel> label(const std::string& text, float fontSize, const morrow::Vector4& color) {
    auto value = std::make_shared<morrow::MRLabel>();
    value->setText(wide(text), "default");
    value->setFontSize(fontSize);
    value->setFontColor(color);
    return value;
}

}  // namespace

namespace morrow::editor {

std::shared_ptr<CreateNodeDialog> CreateNodeDialog::create(const NodeTypeCatalog& catalog) {
    auto dialog = std::shared_ptr<CreateNodeDialog>(new CreateNodeDialog(catalog));
    dialog->initializeControls();
    return dialog;
}

CreateNodeDialog::CreateNodeDialog(const NodeTypeCatalog& catalog) : UIWidget(false), m_catalog(catalog) {
    setWidgetType("EditorCreateNodeDialog");
    setWidgetName("CreateNodeDialog");
    setDisplayLayer(10);
    setClipChildren(true);
    setVisible(false);
    getTransform()->addSizeChangeListener([this]() { layoutControls(); });
}

void CreateNodeDialog::initializeControls() {
    const Vector4 backdropColor(0.015f, 0.02f, 0.03f, 0.58f);
    auto backdropButton = panel(backdropColor, true);
    backdropButton->setHoverColor(backdropColor);
    backdropButton->setPressedColor(backdropColor);
    backdropButton->setDisabledColor(backdropColor);
    m_backdrop = backdropButton;
    addChild(m_backdrop);
    m_panel = panel(Vector4(0.12f, 0.13f, 0.15f, 1.0f));
    addChild(m_panel);

    m_titleLabel = label("Create Child Node", 20.0f, Vector4(0.96f, 0.97f, 0.99f, 1.0f));
    m_panel->addChild(m_titleLabel);
    m_parentLabel = label("Parent:", 13.0f, Vector4(0.58f, 0.68f, 0.80f, 1.0f));
    m_panel->addChild(m_parentLabel);

    m_searchEdit = MRLineEdit::create();
    m_searchEdit->setPlaceholder(L"Search node types");
    m_searchEdit->setFontSize(16.0f);
    m_searchEdit->setBackgroundColor(Vector4(0.075f, 0.085f, 0.105f, 1.0f));
    m_searchEdit->setFocusedBackgroundColor(Vector4(0.10f, 0.13f, 0.18f, 1.0f));
    m_searchEdit->setTextColor(Vector4(0.95f, 0.96f, 0.98f, 1.0f));
    m_searchEdit->setPlaceholderColor(Vector4(0.46f, 0.50f, 0.57f, 1.0f));
    m_searchConnection = m_searchEdit->events().onTextChanged.connect([this](MRTextEdit&, const std::wstring& text) {
        m_filter = narrow(text);
        rebuildTypes();
    });
    m_panel->addChild(m_searchEdit);

    m_typeTree = MRTree::create();
    m_typeTree->setRowHeight(34.0f);
    m_typeTree->setIndentWidth(20.0f);
    MRTree::RowStyle style;
    style.textColor = Vector4(0.82f, 0.85f, 0.90f, 1.0f);
    style.backgroundColor = Vector4(0.075f, 0.08f, 0.095f, 1.0f);
    style.hoverColor = Vector4(0.15f, 0.19f, 0.25f, 1.0f);
    style.selectedTextColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    style.selectedBackgroundColor = Vector4(0.18f, 0.39f, 0.68f, 1.0f);
    style.selectedHoverColor = Vector4(0.23f, 0.48f, 0.78f, 1.0f);
    style.pressedColor = Vector4(0.12f, 0.29f, 0.53f, 1.0f);
    m_typeTree->setRowStyle(style);
    m_treeConnection = m_typeTree->events().onNodeSelected.connect([this](MRTree&, int id, const std::wstring&) {
        const auto iterator = m_typeByTreeId.find(id);
        if (iterator != m_typeByTreeId.end())
            selectType(iterator->second);
    });
    m_panel->addChild(m_typeTree);

    m_descriptionTitle = label("Description", 15.0f, Vector4(0.68f, 0.78f, 0.90f, 1.0f));
    m_panel->addChild(m_descriptionTitle);
    m_descriptionLabel = label("Select a node type to see its description.", 14.0f, Vector4(0.78f, 0.81f, 0.86f, 1.0f));
    m_descriptionLabel->setAutoWrap(true);
    m_panel->addChild(m_descriptionLabel);

    const auto actionButton = [](const std::wstring& text) {
        auto button = MRButton::create();
        button->setText(text, "default");
        button->setTextFontSize(16.0f);
        button->setCornerRadius(4.0f);
        button->setBackgroundColor(Vector4(0.18f, 0.34f, 0.59f, 1.0f));
        button->setHoverColor(Vector4(0.27f, 0.49f, 0.78f, 1.0f));
        button->setPressedColor(Vector4(0.12f, 0.25f, 0.47f, 1.0f));
        return button;
    };
    m_createButton = actionButton(L"Create");
    m_createButton->setEnabled(false);
    m_createConnection = m_createButton->events().onClicked.connect([this](BaseButton&) { confirm(); });
    m_panel->addChild(m_createButton);
    m_cancelButton = actionButton(L"Cancel");
    m_cancelButton->setBackgroundColor(Vector4(0.24f, 0.25f, 0.28f, 1.0f));
    m_cancelButton->setHoverColor(Vector4(0.34f, 0.36f, 0.40f, 1.0f));
    m_cancelConnection = m_cancelButton->events().onClicked.connect([this](BaseButton&) {
        hideDialog();
        m_events.onCanceled.notify(*this);
    });
    m_panel->addChild(m_cancelButton);
    rebuildTypes();
}

void CreateNodeDialog::showForParent(const std::string& parentId, const std::string& parentName) {
    m_parentId = parentId;
    m_parentName = parentName;
    m_selectedType.clear();
    m_filter.clear();
    m_searchEdit->setText(L"");
    m_parentLabel->setText(wide("Parent: " + parentName), "default");
    m_descriptionLabel->setText(L"Select a node type to see its description.", "default");
    m_createButton->setEnabled(false);
    rebuildTypes();
    m_open = true;
    setVisible(true);
    requestRender("show create node dialog");
}

void CreateNodeDialog::hideDialog() {
    m_open = false;
    setVisible(false);
}

bool CreateNodeDialog::isOpen() const {
    return m_open;
}

CreateNodeDialog::Events& CreateNodeDialog::events() {
    return m_events;
}

void CreateNodeDialog::update(FrameStateSharedPtr frameState) {
    layoutControls();
    UIWidget::update(frameState);
}

void CreateNodeDialog::layoutControls() {
    if (!m_panel)
        return;
    const Vector3 size = getTransform()->getSize();
    m_backdrop->getTransform()->setPosition(0.0f, 0.0f, 0.0f);
    m_backdrop->getTransform()->setSize(size.x, size.y);

    const float width = std::min(820.0f, std::max(520.0f, size.x - 100.0f));
    const float height = std::min(620.0f, std::max(440.0f, size.y - 80.0f));
    m_panel->getTransform()->setPosition((size.x - width) * 0.5f, (size.y - height) * 0.5f, 1.0f);
    m_panel->getTransform()->setSize(width, height);
    m_titleLabel->getTransform()->setPosition(20.0f, 14.0f, 0.0f);
    m_titleLabel->getTransform()->setSize(width - 40.0f, 30.0f);
    m_parentLabel->getTransform()->setPosition(20.0f, 46.0f, 0.0f);
    m_parentLabel->getTransform()->setSize(width - 40.0f, 22.0f);
    m_searchEdit->getTransform()->setPosition(20.0f, 76.0f, 0.0f);
    m_searchEdit->getTransform()->setSize(width - 40.0f, 38.0f);
    const float descriptionHeight = 100.0f;
    const float actionsHeight = 54.0f;
    const float treeHeight = std::max(170.0f, height - 130.0f - descriptionHeight - actionsHeight);
    m_typeTree->getTransform()->setPosition(20.0f, 126.0f, 0.0f);
    m_typeTree->getTransform()->setSize(width - 40.0f, treeHeight);
    const float descriptionY = 134.0f + treeHeight;
    m_descriptionTitle->getTransform()->setPosition(20.0f, descriptionY, 0.0f);
    m_descriptionTitle->getTransform()->setSize(width - 40.0f, 22.0f);
    m_descriptionLabel->getTransform()->setPosition(20.0f, descriptionY + 24.0f, 0.0f);
    m_descriptionLabel->getTransform()->setSize(width - 40.0f, descriptionHeight - 28.0f);
    m_createButton->getTransform()->setPosition(width * 0.5f - 190.0f, height - 48.0f, 0.0f);
    m_createButton->getTransform()->setSize(160.0f, 36.0f);
    m_cancelButton->getTransform()->setPosition(width * 0.5f + 30.0f, height - 48.0f, 0.0f);
    m_cancelButton->getTransform()->setSize(160.0f, 36.0f);
}

void CreateNodeDialog::rebuildTypes() {
    if (!m_typeTree)
        return;
    m_typeTree->clear();
    m_typeByTreeId.clear();
    std::map<std::string, int> categories;
    int categoryId = 1000;
    int typeId = 1;
    for (const auto* descriptor : m_catalog.filter(m_filter)) {
        if (!descriptor)
            continue;
        auto category = categories.find(descriptor->category);
        if (category == categories.end()) {
            const int id = categoryId++;
            m_typeTree->addNode(id, wide(descriptor->category), -1, true, false);
            category = categories.emplace(descriptor->category, id).first;
        }
        const int id = typeId++;
        m_typeTree->addNode(id, wide(descriptor->displayName + "  [" + descriptor->type + "]"), category->second, false, true);
        m_typeByTreeId[id] = descriptor->type;
    }
}

void CreateNodeDialog::selectType(const std::string& type) {
    const auto* descriptor = m_catalog.find(type);
    if (!descriptor)
        return;
    m_selectedType = type;
    m_descriptionLabel->setText(wide(descriptor->displayName + " (" + descriptor->type + ")\n" + descriptor->description), "default");
    m_createButton->setEnabled(true);
}

void CreateNodeDialog::confirm() {
    const auto* descriptor = m_catalog.find(m_selectedType);
    if (!descriptor)
        return;
    m_events.onConfirmed.notify(*this, *descriptor, m_parentId);
    hideDialog();
}

}  // namespace morrow::editor
