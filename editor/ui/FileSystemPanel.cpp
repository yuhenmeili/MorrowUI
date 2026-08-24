#include "FileSystemPanel.h"

#include <algorithm>
#include <codecvt>
#include <locale>
#include <sstream>
#include <unordered_map>

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

std::shared_ptr<morrow::MRLabel> makeLabel(
    const std::string& text,
    float fontSize,
    const morrow::Vector4& color) {
    auto label = std::make_shared<morrow::MRLabel>();
    label->setText(wide(text), "default");
    label->setFontSize(fontSize);
    label->setFontColor(color.x, color.y, color.z, color.w);
    return label;
}

int pathDepth(const std::filesystem::path& path) {
    int depth = 0;
    for (const auto& component : path) {
        (void)component;
        ++depth;
    }
    return depth;
}

}  // namespace

namespace morrow::editor {

std::shared_ptr<FileSystemPanel> FileSystemPanel::create(
    ProjectFileSystemModel& model,
    std::function<void(const std::string&)> statusCallback) {
    auto panel = std::shared_ptr<FileSystemPanel>(
        new FileSystemPanel(model, std::move(statusCallback)));
    panel->initializeControls();
    panel->refreshView();
    return panel;
}

FileSystemPanel::FileSystemPanel(
    ProjectFileSystemModel& model,
    std::function<void(const std::string&)> statusCallback)
    : UIWidget(false),
      m_model(model),
      m_statusCallback(std::move(statusCallback)) {
    setWidgetType("EditorFileSystemPanel");
    setWidgetName("FileSystem");
    setClipChildren(true);
    getTransform()->addSizeChangeListener([this]() { layoutControls(); });
}

void FileSystemPanel::initializeControls() {
    m_titleLabel = makeLabel(
        "FileSystem", 16.0f, Vector4(0.93f, 0.95f, 0.98f, 1.0f));
    addChild(m_titleLabel);

    m_refreshButton = MRButton::create();
    m_refreshButton->setText(L"Refresh", "default");
    m_refreshButton->setTextFontSize(14.0f);
    m_refreshButton->setCornerRadius(2.0f);
    m_refreshButton->setBackgroundColor(
        Vector4(0.16f, 0.31f, 0.55f, 1.0f));
    m_refreshButton->setHoverColor(
        Vector4(0.25f, 0.48f, 0.76f, 1.0f));
    m_refreshButton->setPressedColor(
        Vector4(0.10f, 0.23f, 0.44f, 1.0f));
    m_refreshConnection =
        m_refreshButton->events().onClicked.connect(
            [this](BaseButton&) { refreshModel(); });
    addChild(m_refreshButton);

    m_searchEdit = MRLineEdit::create();
    m_searchEdit->setPlaceholder(L"Filter files");
    m_searchEdit->setFontSize(14.0f);
    m_searchEdit->setBackgroundColor(
        Vector4(0.10f, 0.12f, 0.15f, 1.0f));
    m_searchEdit->setFocusedBackgroundColor(
        Vector4(0.14f, 0.18f, 0.24f, 1.0f));
    m_searchEdit->setTextColor(
        Vector4(0.93f, 0.95f, 0.98f, 1.0f));
    m_searchEdit->setPlaceholderColor(
        Vector4(0.48f, 0.54f, 0.62f, 1.0f));
    m_searchConnection =
        m_searchEdit->events().onTextChanged.connect(
            [this](MRTextEdit&, const std::wstring& text) {
                m_filter = narrow(text);
                rebuildTree();
            });
    addChild(m_searchEdit);

    m_pathLabel = makeLabel(
        "res://", 13.0f, Vector4(0.55f, 0.68f, 0.82f, 1.0f));
    addChild(m_pathLabel);

    m_tree = MRTree::create();
    m_tree->setRowHeight(28.0f);
    m_tree->setIndentWidth(16.0f);
    MRTree::RowStyle treeStyle;
    treeStyle.textColor = Vector4(0.82f, 0.86f, 0.91f, 1.0f);
    treeStyle.backgroundColor = Vector4(0.13f, 0.15f, 0.19f, 1.0f);
    treeStyle.hoverColor = Vector4(0.20f, 0.25f, 0.33f, 1.0f);
    treeStyle.selectedTextColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    treeStyle.selectedBackgroundColor =
        Vector4(0.16f, 0.34f, 0.62f, 1.0f);
    treeStyle.selectedHoverColor =
        Vector4(0.20f, 0.42f, 0.72f, 1.0f);
    treeStyle.pressedColor = Vector4(0.10f, 0.25f, 0.48f, 1.0f);
    m_tree->setRowStyle(treeStyle);
    m_selectionConnection =
        m_tree->events().onNodeSelected.connect(
            [this](MRTree&, int id, const std::wstring&) {
                handleSelection(id);
            });
    addChild(m_tree);

    m_statusLabel = makeLabel(
        m_status, 12.0f, Vector4(0.60f, 0.66f, 0.74f, 1.0f));
    addChild(m_statusLabel);
    layoutControls();
}

void FileSystemPanel::refreshView() {
    rebuildTree();
    layoutControls();
}

void FileSystemPanel::update(FrameStateSharedPtr frameState) {
    layoutControls();
    UIWidget::update(frameState);
}

void FileSystemPanel::layoutControls() {
    if (!m_tree)
        return;
    const Vector3 size = getTransform()->getSize();
    const float width = std::max(1.0f, size.x);
    const float treeHeight = std::max(1.0f, size.y - 108.0f);
    m_titleLabel->getTransform()->setPosition(8.0f, 4.0f, 0.0f);
    m_titleLabel->getTransform()->setSize(
        std::max(1.0f, width - 92.0f), 24.0f);
    m_refreshButton->getTransform()->setPosition(
        std::max(8.0f, width - 78.0f), 3.0f, 0.0f);
    m_refreshButton->getTransform()->setSize(70.0f, 26.0f);
    m_searchEdit->getTransform()->setPosition(8.0f, 34.0f, 0.0f);
    m_searchEdit->getTransform()->setSize(
        std::max(1.0f, width - 16.0f), 30.0f);
    m_pathLabel->getTransform()->setPosition(8.0f, 68.0f, 0.0f);
    m_pathLabel->getTransform()->setSize(
        std::max(1.0f, width - 16.0f), 20.0f);
    m_tree->getTransform()->setPosition(8.0f, 90.0f, 0.0f);
    m_tree->getTransform()->setSize(
        std::max(1.0f, width - 16.0f), treeHeight);
    m_statusLabel->getTransform()->setPosition(
        8.0f, std::max(90.0f, size.y - 17.0f), 0.0f);
    m_statusLabel->getTransform()->setSize(
        std::max(1.0f, width - 16.0f), 16.0f);
}

void FileSystemPanel::rebuildTree() {
    if (!m_tree)
        return;

    std::unordered_map<int, bool> expanded;
    for (const auto& node : m_tree->getNodes())
        expanded[node.id] = node.expanded;

    m_tree->clear();
    const auto visibleEntries = m_model.filteredEntries(m_filter);
    for (const auto* entry : visibleEntries) {
        if (!entry)
            continue;
        bool isExpanded = entry->directory &&
                          pathDepth(entry->relativePath) <= 1;
        if (const auto iterator = expanded.find(entry->id);
            iterator != expanded.end()) {
            isExpanded = iterator->second;
        }
        m_tree->addNode(
            entry->id,
            entryText(*entry),
            entry->parentId,
            isExpanded,
            true);
    }

    std::ostringstream status;
    status << m_model.entries().size() << " entries";
    if (!m_filter.empty())
        status << ", " << visibleEntries.size() << " visible";
    setPanelStatus(status.str());
}

void FileSystemPanel::refreshModel() {
    std::string error;
    if (!m_model.refresh(error)) {
        setPanelStatus("Refresh failed");
        if (m_statusCallback)
            m_statusCallback(error);
        return;
    }
    rebuildTree();
    if (m_statusCallback)
        m_statusCallback(
            "FileSystem refreshed: " +
            std::to_string(m_model.entries().size()) + " entries");
}

void FileSystemPanel::handleSelection(int id) {
    const auto* entry = m_model.findById(id);
    if (!entry)
        return;
    if (entry->directory) {
        setPanelStatus("res://" + entry->relativePath.generic_string());
        return;
    }

    std::string status = "res://" + entry->relativePath.generic_string();
    if (!entry->assetType.empty())
        status += " [" + entry->assetType + "]";
    if (entry->importState != FileImportState::NotApplicable)
        status += " " + std::string(fileImportStateName(entry->importState));
    setPanelStatus(status);
}

std::wstring FileSystemPanel::entryText(
    const ProjectFileEntry& entry) const {
    if (entry.directory)
        return wide(entry.name);
    std::string text = entry.name;
    if (!entry.assetType.empty())
        text += "  [" + entry.assetType + "]";
    if (entry.importState == FileImportState::NeedsImport)
        text += " *";
    else if (entry.importState == FileImportState::Failed)
        text += " !";
    return wide(text);
}

void FileSystemPanel::setPanelStatus(const std::string& status) {
    m_status = status;
    if (m_statusLabel)
        m_statusLabel->setText(wide(status), "default");
}

}  // namespace morrow::editor
