#include "FileSystemPanel.h"

#include <algorithm>
#include <codecvt>
#include <locale>
#include <sstream>
#include <unordered_map>

#include "base/BaseButton.h"
#include "base/Interaction.h"
#include "base/TouchEvent.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "elements/MRImage.h"
#include "elements/MRLabel.h"
#include "elements/MRLineEdit.h"
#include "elements/MRScrollContainer.h"

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

std::shared_ptr<morrow::MRLabel> makeLabel(const std::string& text, float fontSize, const morrow::Vector4& color) {
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

std::shared_ptr<FileSystemPanel> FileSystemPanel::create(ProjectFileSystemModel& model, std::function<void(const std::string&)> statusCallback,
                                                         std::function<void()> assetChangeCallback,
                                                         std::function<void(const ProjectFileEntry&)> selectionCallback) {
    auto panel = std::shared_ptr<FileSystemPanel>(new FileSystemPanel(model, std::move(statusCallback), std::move(assetChangeCallback)));
    panel->m_selectionCallback = std::move(selectionCallback);
    panel->initializeControls();
    panel->refreshView();
    return panel;
}

FileSystemPanel::FileSystemPanel(ProjectFileSystemModel& model, std::function<void(const std::string&)> statusCallback, std::function<void()> assetChangeCallback) :
    UIWidget(false), m_model(model), m_statusCallback(std::move(statusCallback)), m_assetChangeCallback(std::move(assetChangeCallback)) {
    setWidgetType("EditorFileSystemPanel");
    setWidgetName("FileSystem");
    setClipChildren(true);
    getTransform()->addSizeChangeListener([this]() { layoutControls(); });
}

bool FileSystemPanel::selectAsset(const std::string& assetId, bool notify) {
    for (const auto& entry : m_model.entries()) {
        if (entry.assetId != assetId)
            continue;
        m_model.select(entry.id, false);
        if (m_tree)
            m_tree->selectNode(entry.id, notify);
        updateCardStyles();
        if (notify)
            handleSelection(entry.id);
        return true;
    }
    return false;
}

std::vector<const ProjectFileEntry*> FileSystemPanel::selectedEntries() const {
    if (m_gridMode)
        return m_model.selectedEntries();
    std::vector<const ProjectFileEntry*> result;
    if (m_tree) {
        if (const auto* entry = m_model.findById(m_tree->getSelectedId()))
            result.push_back(entry);
    }
    return result;
}

void FileSystemPanel::initializeControls() {
    m_titleLabel = makeLabel("FileSystem", 16.0f, Vector4(0.93f, 0.95f, 0.98f, 1.0f));
    addChild(m_titleLabel);

    m_refreshButton = MRButton::create();
    m_refreshButton->setText(L"Refresh", "default");
    m_refreshButton->setTextFontSize(14.0f);
    m_refreshButton->setCornerRadius(2.0f);
    m_refreshButton->setBackgroundColor(Vector4(0.16f, 0.31f, 0.55f, 1.0f));
    m_refreshButton->setHoverColor(Vector4(0.25f, 0.48f, 0.76f, 1.0f));
    m_refreshButton->setPressedColor(Vector4(0.10f, 0.23f, 0.44f, 1.0f));
    m_refreshConnection = m_refreshButton->events().onClicked.connect([this](BaseButton&) { refreshModel(); });
    addChild(m_refreshButton);

    const auto makeToolbarButton = [](const std::wstring& text) {
        auto button = MRButton::create();
        button->setText(text, "default");
        button->setTextFontSize(12.0f);
        button->setCornerRadius(2.0f);
        button->setBackgroundColor(Vector4(0.13f, 0.20f, 0.31f, 1.0f));
        button->setHoverColor(Vector4(0.21f, 0.34f, 0.50f, 1.0f));
        button->setPressedColor(Vector4(0.09f, 0.17f, 0.28f, 1.0f));
        return button;
    };
    m_sortButton = makeToolbarButton(L"Sort: Name");
    m_sortConnection = m_sortButton->events().onClicked.connect([this](BaseButton&) { cycleSort(); });
    addChild(m_sortButton);
    m_viewButton = makeToolbarButton(L"Grid");
    m_viewConnection = m_viewButton->events().onClicked.connect([this](BaseButton&) { toggleView(); });
    addChild(m_viewButton);
    m_backButton = makeToolbarButton(L"Up");
    m_backConnection = m_backButton->events().onClicked.connect([this](BaseButton&) { navigateBack(); });
    addChild(m_backButton);

    m_searchEdit = MRLineEdit::create();
    m_searchEdit->setPlaceholder(L"Filter files");
    m_searchEdit->setFontSize(14.0f);
    m_searchEdit->setBackgroundColor(Vector4(0.10f, 0.12f, 0.15f, 1.0f));
    m_searchEdit->setFocusedBackgroundColor(Vector4(0.14f, 0.18f, 0.24f, 1.0f));
    m_searchEdit->setTextColor(Vector4(0.93f, 0.95f, 0.98f, 1.0f));
    m_searchEdit->setPlaceholderColor(Vector4(0.48f, 0.54f, 0.62f, 1.0f));
    m_searchConnection = m_searchEdit->events().onTextChanged.connect([this](MRTextEdit&, const std::wstring& text) {
        m_filter = narrow(text);
        rebuildTree();
        rebuildGrid();
    });
    addChild(m_searchEdit);

    m_pathLabel = makeLabel("res://", 13.0f, Vector4(0.55f, 0.68f, 0.82f, 1.0f));
    addChild(m_pathLabel);

    m_tree = MRTree::create();
    m_tree->setRowHeight(28.0f);
    m_tree->setIndentWidth(16.0f);
    MRTree::RowStyle treeStyle;
    treeStyle.textColor = Vector4(0.82f, 0.86f, 0.91f, 1.0f);
    treeStyle.backgroundColor = Vector4(0.13f, 0.15f, 0.19f, 1.0f);
    treeStyle.hoverColor = Vector4(0.20f, 0.25f, 0.33f, 1.0f);
    treeStyle.selectedTextColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    treeStyle.selectedBackgroundColor = Vector4(0.16f, 0.34f, 0.62f, 1.0f);
    treeStyle.selectedHoverColor = Vector4(0.20f, 0.42f, 0.72f, 1.0f);
    treeStyle.pressedColor = Vector4(0.10f, 0.25f, 0.48f, 1.0f);
    m_tree->setRowStyle(treeStyle);
    m_selectionConnection = m_tree->events().onNodeSelected.connect([this](MRTree&, int id, const std::wstring&) { handleSelection(id); });
    addChild(m_tree);

    m_gridScroll = MRScrollContainer::create();
    m_gridContent = std::make_shared<UIWidget>(false);
    m_gridScroll->setContent(m_gridContent);
    m_gridScroll->setVisible(false);
    addChild(m_gridScroll);

    m_statusLabel = makeLabel(m_status, 12.0f, Vector4(0.60f, 0.66f, 0.74f, 1.0f));
    addChild(m_statusLabel);
    std::string watcherError;
    if (!m_model.projectRoot().empty() && !m_watcher.start(m_model.projectRoot(), watcherError)) {
        setPanelStatus("Watcher unavailable");
    }
    layoutControls();
}

void FileSystemPanel::refreshView() {
    rebuildTree();
    rebuildGrid();
    layoutControls();
}

const ProjectFileEntry* FileSystemPanel::entryForWidget(const std::shared_ptr<Widget>& widget) const {
    if (!widget)
        return nullptr;

    if (m_tree) {
        const int treeId = m_tree->nodeIdForWidget(widget);
        if (treeId >= 0)
            return m_model.findById(treeId);
    }

    for (auto current = widget; current && current.get() != this; current = current->m_parent) {
        for (const auto& [entryId, card] : m_gridCards) {
            if (card == current)
                return m_model.findById(entryId);
        }
    }
    return nullptr;
}

void FileSystemPanel::update(FrameStateSharedPtr frameState) {
    const auto changes = m_watcher.poll();
    if (!changes.empty()) {
        for (const auto& change : changes)
            m_thumbnails.invalidate(m_model.projectRoot() / change.relativePath);
        std::string error;
        if (m_model.refresh(error)) {
            rebuildTree();
            rebuildGrid();
            setPanelStatus(std::to_string(changes.size()) + " file change(s)");
        } else if (m_statusCallback) {
            m_statusCallback(error);
        }
        if (m_assetChangeCallback)
            m_assetChangeCallback();
    }
    if (m_thumbnails.poll() > 0 && m_gridMode)
        rebuildGrid();
    layoutControls();
    UIWidget::update(frameState);
}

void FileSystemPanel::layoutControls() {
    if (!m_tree)
        return;
    const Vector3 size = getTransform()->getSize();
    const float width = std::max(1.0f, size.x);
    const float contentHeight = std::max(1.0f, size.y - 140.0f);
    m_titleLabel->getTransform()->setPosition(8.0f, 4.0f, 0.0f);
    m_titleLabel->getTransform()->setSize(std::max(1.0f, width - 298.0f), 24.0f);
    m_backButton->getTransform()->setPosition(std::max(8.0f, width - 290.0f), 3.0f, 0.0f);
    m_backButton->getTransform()->setSize(48.0f, 26.0f);
    m_sortButton->getTransform()->setPosition(std::max(60.0f, width - 238.0f), 3.0f, 0.0f);
    m_sortButton->getTransform()->setSize(92.0f, 26.0f);
    m_viewButton->getTransform()->setPosition(std::max(156.0f, width - 142.0f), 3.0f, 0.0f);
    m_viewButton->getTransform()->setSize(60.0f, 26.0f);
    m_refreshButton->getTransform()->setPosition(std::max(8.0f, width - 78.0f), 3.0f, 0.0f);
    m_refreshButton->getTransform()->setSize(70.0f, 26.0f);
    m_searchEdit->getTransform()->setPosition(8.0f, 34.0f, 0.0f);
    m_searchEdit->getTransform()->setSize(std::max(1.0f, width - 16.0f), 30.0f);
    m_pathLabel->getTransform()->setPosition(8.0f, 68.0f, 0.0f);
    m_pathLabel->getTransform()->setSize(std::max(1.0f, width - 16.0f), 20.0f);
    m_tree->getTransform()->setPosition(8.0f, 90.0f, 0.0f);
    m_tree->getTransform()->setSize(std::max(1.0f, width - 16.0f), contentHeight);
    m_gridScroll->getTransform()->setPosition(8.0f, 90.0f, 0.0f);
    m_gridScroll->getTransform()->setSize(std::max(1.0f, width - 16.0f), contentHeight);
    m_statusLabel->getTransform()->setPosition(8.0f, std::max(90.0f, size.y - 17.0f), 0.0f);
    m_statusLabel->getTransform()->setSize(std::max(1.0f, width - 16.0f), 16.0f);
}

void FileSystemPanel::rebuildGrid() {
    if (!m_gridContent)
        return;
    m_gridScroll->clearScrollChildren();
    m_gridConnections.clear();
    m_gridCards.clear();
    m_gridImages.clear();

    const auto* directory = m_model.findByPath(m_currentDirectory);
    if (!directory || !directory->directory) {
        m_currentDirectory = ".";
        directory = m_model.findByPath(m_currentDirectory);
    }
    if (!directory)
        return;

    std::unordered_map<int, bool> visibleIds;
    for (const auto* entry : m_model.filteredEntries(m_filter))
        visibleIds[entry->id] = true;
    const float availableWidth = std::max(100.0f, m_gridScroll->getTransform()->getSize().x - 20.0f);
    constexpr float cardWidth = 104.0f;
    constexpr float cardHeight = 96.0f;
    constexpr float gap = 8.0f;
    const int columns = std::max(1, static_cast<int>((availableWidth + gap) / (cardWidth + gap)));
    int cardIndex = 0;
    for (const auto& entry : m_model.entries()) {
        if (entry.parentId != directory->id || visibleIds.count(entry.id) == 0)
            continue;
        auto card = MRButton::create();
        card->setText(entryText(entry), "default");
        card->setTextFontSize(12.0f);
        card->setTextAlign(HorizontalAlignment::CENTER, VerticalAlignment::BOTTOM);
        card->setCornerRadius(4.0f);
        const int entryId = entry.id;
        auto interaction = card->getComponent<Interaction>();
        m_gridConnections.emplace_back(
            interaction->addEventListener(TOUCH_EVENT_TYPE_CLICK, [this, entryId](TouchEvent& event) { handleGridClick(entryId, event.modifiers); }, 10));
        m_gridConnections.emplace_back(
            interaction->addEventListener(TOUCH_EVENT_TYPE_TOUCH, [this, entryId](TouchEvent& event) {
                if (event.button == TOUCH_MOUSE_BUTTON_RIGHT && m_contextMenuCallback)
                    m_contextMenuCallback(m_model.findById(entryId), event.positionX, event.positionY);
            }, 20));

        if (!entry.directory) {
            const auto absolute = m_model.projectRoot() / entry.relativePath;
            const auto thumbnail = m_thumbnails.request(absolute);
            if (thumbnail.state == ThumbnailState::Ready && thumbnail.texture) {
                auto image = MRImage::create();
                image->setTexture(thumbnail.texture);
                image->setRounding(4.0f);
                image->getTransform()->setPosition(8.0f, 8.0f, 1.0f);
                image->getTransform()->setSize(72.0f, 58.0f);
                card->addChild(image);
                m_gridImages.push_back({absolute, image});
            }
        }
        const int column = cardIndex % columns;
        const int row = cardIndex / columns;
        card->getTransform()->setPosition(static_cast<float>(column) * (cardWidth + gap), static_cast<float>(row) * (cardHeight + gap), 0.0f);
        card->getTransform()->setSize(cardWidth, cardHeight);
        m_gridScroll->addScrollChild(card);
        m_gridCards.push_back({entry.id, card});
        ++cardIndex;
    }
    const int rows = cardIndex == 0 ? 0 : (cardIndex + columns - 1) / columns;
    m_gridContent->getTransform()->setSize(availableWidth, std::max(1.0f, static_cast<float>(rows) * (cardHeight + gap)));
    updateCardStyles();
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
        bool isExpanded = entry->directory && pathDepth(entry->relativePath) <= 1;
        if (const auto iterator = expanded.find(entry->id); iterator != expanded.end()) {
            isExpanded = iterator->second;
        }
        m_tree->addNode(entry->id, entryText(*entry), entry->parentId, isExpanded, true);
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
    rebuildGrid();
    if (m_statusCallback)
        m_statusCallback("FileSystem refreshed: " + std::to_string(m_model.entries().size()) + " entries");
}

void FileSystemPanel::handleSelection(int id) {
    const auto* entry = m_model.findById(id);
    if (!entry)
        return;
    if (m_selectionCallback)
        m_selectionCallback(*entry);
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

void FileSystemPanel::handleGridClick(int id, uint32_t modifiers) {
    const auto* entry = m_model.findById(id);
    if (!entry)
        return;
    if (entry->directory && (modifiers & TOUCH_MODIFIER_CTRL) == 0) {
        m_currentDirectory = entry->relativePath;
        m_pathLabel->setText(wide("res://" + m_currentDirectory.generic_string()), "default");
        rebuildGrid();
        return;
    }
    m_model.select(id, (modifiers & TOUCH_MODIFIER_CTRL) != 0);
    updateCardStyles();
    const auto selected = m_model.selectedEntries();
    if (m_selectionCallback)
        m_selectionCallback(*entry);
    setPanelStatus(std::to_string(selected.size()) + " selected");
}

void FileSystemPanel::cycleSort() {
    FileSortMode next = FileSortMode::Name;
    switch (m_model.sortMode()) {
        case FileSortMode::Name:
            next = FileSortMode::Type;
            break;
        case FileSortMode::Type:
            next = FileSortMode::Modified;
            break;
        case FileSortMode::Modified:
            next = FileSortMode::Size;
            break;
        case FileSortMode::Size:
            next = FileSortMode::Name;
            break;
    }
    m_model.setSortMode(next, true);
    std::string error;
    if (!m_model.refresh(error)) {
        if (m_statusCallback)
            m_statusCallback(error);
        return;
    }
    m_sortButton->setText(wide("Sort: " + std::string(fileSortModeName(next))), "default");
    rebuildTree();
    rebuildGrid();
}

void FileSystemPanel::toggleView() {
    m_gridMode = !m_gridMode;
    m_tree->setVisible(!m_gridMode);
    m_gridScroll->setVisible(m_gridMode);
    m_viewButton->setText(m_gridMode ? L"Tree" : L"Grid", "default");
    if (m_gridMode)
        rebuildGrid();
}

void FileSystemPanel::navigateBack() {
    if (m_currentDirectory == "." || m_currentDirectory.empty())
        return;
    m_currentDirectory = m_currentDirectory.parent_path();
    if (m_currentDirectory.empty())
        m_currentDirectory = ".";
    m_pathLabel->setText(wide("res://" + m_currentDirectory.generic_string()), "default");
    rebuildGrid();
}

void FileSystemPanel::updateCardStyles() {
    for (auto& [id, card] : m_gridCards) {
        const auto* entry = m_model.findById(id);
        const bool selected = entry && m_model.isSelected(entry->relativePath);
        card->setTextColor(selected ? Vector4(1.0f, 1.0f, 1.0f, 1.0f) : Vector4(0.82f, 0.86f, 0.91f, 1.0f));
        card->setBackgroundColor(selected ? Vector4(0.16f, 0.36f, 0.66f, 1.0f) : Vector4(0.13f, 0.15f, 0.19f, 1.0f));
        card->setHoverColor(selected ? Vector4(0.22f, 0.46f, 0.78f, 1.0f) : Vector4(0.20f, 0.25f, 0.33f, 1.0f));
    }
}

std::wstring FileSystemPanel::entryText(const ProjectFileEntry& entry) const {
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
