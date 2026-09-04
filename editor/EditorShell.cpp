#include "EditorShell.h"

#include <algorithm>
#include <chrono>
#include <codecvt>
#include <fstream>
#include <iomanip>
#include <locale>
#include <utility>

#include "Engine.h"
#include "base/Interaction.h"
#include "base/TouchEvent.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "elements/MRLabel.h"
#include "elements/MRLineEdit.h"
#include "elements/MRPopupMenu.h"
#include "layout/EditorLayoutController.h"
#include "assets/AssetTypeCatalog.h"
#include "panels/AssetBrowserPanel.h"
#include "panels/BuildPanel.h"
#include "panels/InspectorPanel.h"
#include "panels/OutputPanel.h"
#include "panels/SceneTreePanel.h"
#include "panels/ToolbarPanel.h"
#include "panels/ViewportPanel.h"
#include "ui/CreateAssetDialog.h"
#include "ui/RenameNodeDialog.h"
#include "platform/Window.h"
#include "wgl/OpenglHeader.h"

namespace {
std::unordered_map<GLFWwindow*, morrow::editor::EditorShell*>& shells() {
    static std::unordered_map<GLFWwindow*, morrow::editor::EditorShell*> value;
    return value;
}

std::unordered_map<GLFWwindow*, GLFWkeyfun>& previousKeyCallbacks() {
    static std::unordered_map<GLFWwindow*, GLFWkeyfun> callbacks;
    return callbacks;
}

std::unordered_map<GLFWwindow*, GLFWcharfun>& previousCharCallbacks() {
    static std::unordered_map<GLFWwindow*, GLFWcharfun> callbacks;
    return callbacks;
}

std::shared_ptr<morrow::UIWidget> makePanel(float x, float y, float width, float height, const morrow::Math::Vector4& color = morrow::Math::Vector4(0.12f, 0.14f, 0.17f, 1.0f)) {
    auto panel = morrow::MRButton::create();
    panel->setInteractive(false);
    panel->setBackgroundColor(color);
    panel->setCornerRadius(0.0f);
    panel->setClipChildren(true);
    auto transform = panel->getTransform();
    transform->setPosition(x, y, 0.0f);
    transform->setSize(width, height);
    return panel;
}

bool isDescendantOf(std::shared_ptr<morrow::Widget> widget, const std::shared_ptr<morrow::Widget>& ancestor) {
    while (widget) {
        if (widget == ancestor)
            return true;
        widget = widget->m_parent;
    }
    return false;
}

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

}  // namespace

namespace morrow::editor {

EditorShell::EditorShell(const std::shared_ptr<Window>& window, const std::shared_ptr<Engine>& engine, std::filesystem::path projectPath, std::filesystem::path scenePath,
                         std::filesystem::path assetRoot) :
    m_window(window), m_engine(engine), m_projectPath(std::move(projectPath)), m_scenePath(std::move(scenePath)), m_assetRoot(std::move(assetRoot)),
    m_inspector(std::make_unique<InspectorPanel>(*this)), m_sceneTree(std::make_unique<SceneTreePanel>(*this)), m_viewport(std::make_unique<ViewportPanel>(*this)),
    m_toolbar(std::make_unique<ToolbarPanel>(*this)), m_assetsPanel(std::make_unique<AssetBrowserPanel>(*this)), m_output(std::make_unique<OutputPanel>(*this)),
    m_build(std::make_unique<BuildPanel>(*this)), m_layout(std::make_unique<EditorLayoutController>(*this)), m_session(std::make_shared<EditorSession>(m_scenePath)),
    m_dockLayoutPath(m_projectPath.parent_path() / ".morrow" / "editor.layout") {
    m_selectionChangedConnection = m_events.onSelectionChanged.connect([this](const SelectionState& selection) {
        m_selectedNodeId = selection.nodeIds.empty() ? std::string{} : selection.nodeIds.back();
        m_viewport->refreshSelectionOverlay();
        m_inspector->refresh();
    });
}

EditorShell::~EditorShell() {
    if (m_buildFuture.valid())
        m_buildFuture.wait();
    m_inputConnection.disconnect();
    if (m_window) {
        auto* glfwWindow = static_cast<GLFWwindow*>(m_window->getSurface());
        if (glfwWindow) {
            if (const auto iterator = previousKeyCallbacks().find(glfwWindow); iterator != previousKeyCallbacks().end()) {
                glfwSetKeyCallback(glfwWindow, iterator->second);
                previousKeyCallbacks().erase(iterator);
            }
            if (const auto iterator = previousCharCallbacks().find(glfwWindow); iterator != previousCharCallbacks().end()) {
                glfwSetCharCallback(glfwWindow, iterator->second);
                previousCharCallbacks().erase(iterator);
            }
            shells().erase(glfwWindow);
        }
    }
}

EditorEvents& EditorShell::events() {
    return m_events;
}

void EditorShell::setStatus(const std::string& text) {
    m_status = text;
    m_outputLines.push_back(text);
    if (m_outputLines.size() > 6)
        m_outputLines.erase(m_outputLines.begin());
    m_output->lines = m_outputLines;
    m_output->refresh();
}

bool EditorShell::notifySelectionChanged() {
    const auto& selection = m_session->model().selection();
    if (selection.nodeIds == m_lastNotifiedSelection) {
        return false;
    }
    m_lastNotifiedSelection = selection.nodeIds;
    m_events.onSelectionChanged.notify(selection);
    m_sceneTree->refreshSelectionStyles();
    return true;
}

void EditorShell::addLabel(const std::shared_ptr<UIWidget>& parent, const std::string& text, float x, float y, float width, float height) {
    auto label = std::make_shared<MRLabel>();
    label->setText(std::wstring(text.begin(), text.end()), "default");
    label->setFontSize(16.0f);
    label->setFontColor(1.0f, 1.0f, 1.0f, 1.0f);
    auto transform = label->getComponent<Transform>();
    transform->setPosition(x, y, 0.0f);
    transform->setSize(width, height);
    parent->addChild(label);
}

std::shared_ptr<MRButton> EditorShell::addButton(const std::shared_ptr<UIWidget>& parent, const std::wstring& text, float x, float y, float width, float height,
                                                 std::function<void()> callback) {
    auto button = MRButton::create();
    button->setText(text, "default");
    button->setTextFontSize(16.0f);
    button->setAutoWrap(false);
    button->setBackgroundColor(morrow::Math::Vector4(0.16f, 0.31f, 0.63f, 1.0f));
    button->setHoverColor(morrow::Math::Vector4(0.30f, 0.55f, 0.95f, 1.0f));
    button->setPressedColor(morrow::Math::Vector4(0.10f, 0.22f, 0.48f, 1.0f));
    m_buttonConnections.erase(
        std::remove_if(m_buttonConnections.begin(), m_buttonConnections.end(), [](const Observable<BaseButton&>::Connection& connection) { return !connection.connected(); }),
        m_buttonConnections.end());
    m_buttonConnections.emplace_back(button->events().onClicked.connect([callback = std::move(callback)](BaseButton&) {
        if (callback) {
            callback();
        }
    }));
    auto transform = button->getComponent<Transform>();
    transform->setPosition(x, y, 0.0f);
    transform->setSize(width, height);
    parent->addChild(button);
    return button;
}

void EditorShell::buildLayout() {
    m_shellRoot = std::make_shared<UIWidget>(false);
    m_shellRoot->setWidgetName("EditorShell");
    m_shellRoot->getTransform()->setPosition(0.0f, 0.0f, 100.0f);
    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    if (m_window && m_window->getSurface()) {
        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(static_cast<GLFWwindow*>(m_window->getSurface()), &framebufferWidth, &framebufferHeight);
        if (framebufferWidth > 0 && framebufferHeight > 0) {
            windowWidth = static_cast<float>(framebufferWidth);
            windowHeight = static_cast<float>(framebufferHeight);
        }
    }
    m_shellRoot->getTransform()->setSize(windowWidth, windowHeight);

    const DockLayout defaultLayout = DockLayout::defaultLayout(windowWidth, windowHeight);
    m_dockLayout = defaultLayout;
    std::string layoutError;
    if (std::filesystem::exists(m_dockLayoutPath)) {
        DockLayout loadedLayout;
        if (loadedLayout.load(m_dockLayoutPath, layoutError)) {
            for (const auto& defaultSplit : defaultLayout.splits()) {
                if (!loadedLayout.findSplit(defaultSplit.id))
                    loadedLayout.splits().push_back(defaultSplit);
            }
            for (const auto& defaultTabs : defaultLayout.tabs()) {
                if (!loadedLayout.findTabs(defaultTabs.id))
                    loadedLayout.tabs().push_back(defaultTabs);
            }
            m_dockLayout = std::move(loadedLayout);
        }
    }
    // FileSystem belongs to the bottom tool area by default. Migrate older
    // layouts that still placed it in the left dock without changing the
    // user's other tab ordering.
    bool migratedFileSystemDock = false;
    if (auto* leftTabs = m_dockLayout.findTabs("left_dock")) {
        migratedFileSystemDock = std::find(leftTabs->panels.begin(), leftTabs->panels.end(), "filesystem") != leftTabs->panels.end();
        leftTabs->panels.erase(std::remove(leftTabs->panels.begin(), leftTabs->panels.end(), "filesystem"), leftTabs->panels.end());
    }
    if (auto* bottomTabs = m_dockLayout.findTabs("bottom_dock")) {
        if (std::find(bottomTabs->panels.begin(), bottomTabs->panels.end(), "filesystem") == bottomTabs->panels.end())
            bottomTabs->panels.push_back("filesystem");
        if (migratedFileSystemDock)
            bottomTabs->active = "output";
    }

    m_toolbar->panel = makePanel(0.0f, 0.0f, windowWidth, 40.0f);
    m_sceneTree->panel = makePanel(0.0f, 0.0f, 240.0f, 570.0f, morrow::Math::Vector4(0.13f, 0.15f, 0.19f, 1.0f));
    m_viewport->panel = makePanel(0.0f, 0.0f, 740.0f, 570.0f, morrow::Math::Vector4(0.10f, 0.12f, 0.15f, 1.0f));
    m_inspector->panel = makePanel(0.0f, 0.0f, 300.0f, 570.0f, morrow::Math::Vector4(0.13f, 0.15f, 0.19f, 1.0f));
    m_output->panel = makePanel(0.0f, 0.0f, 1280.0f, 140.0f, morrow::Math::Vector4(0.11f, 0.13f, 0.16f, 1.0f));
    m_build->panel = makePanel(0.0f, 0.0f, 1280.0f, 140.0f, morrow::Math::Vector4(0.11f, 0.13f, 0.16f, 1.0f));
    m_assetsPanel->view = FileSystemPanel::create(
        m_fileSystem,
        [this](const std::string& status) { setStatus(status); },
        [this]() { m_assetsPanel->importAssets(); },
        [this](const ProjectFileEntry& entry) { m_inspector->inspectAsset(entry); });
    m_viewport->previewRoot = makePanel(0.0f, 32.0f, 740.0f, 538.0f);
    m_viewport->previewRoot->setWidgetName("PreviewRoot");

    m_workspaceSplit = MRSplitContainer::create();
    m_workspaceSplit->setOrientation(SplitOrientation::Vertical);
    m_workspaceSplit->setFirstMinSize(260.0f);
    m_workspaceSplit->setSecondMinSize(120.0f);
    m_workspaceSplit->setHandleWidth(5.0f);
    m_workspaceSplit->getTransform()->setPosition(0.0f, 40.0f, 0.0f);
    m_workspaceSplit->getTransform()->setSize(windowWidth, std::max(1.0f, windowHeight - 40.0f));

    m_mainSplit = MRSplitContainer::create();
    m_mainSplit->setOrientation(SplitOrientation::Horizontal);
    m_mainSplit->setFirstMinSize(220.0f);
    m_mainSplit->setSecondMinSize(520.0f);
    m_mainSplit->setHandleWidth(5.0f);

    m_leftTabs = MRTabContainer::create();
    m_leftTabs->addTab("scene_tree", L"Scene", m_sceneTree->panel);

    m_centerTabs = MRTabContainer::create();
    m_centerTabs->addTab("viewport", L"Viewport", m_viewport->panel);

    m_bottomTabs = MRTabContainer::create();
    m_bottomTabs->addTab("output", L"Output", m_output->panel);
    m_bottomTabs->addTab("build", L"Build", m_build->panel);
    m_bottomTabs->addTab("filesystem", L"FileSystem", m_assetsPanel->view);
    m_dockDropOverlay = DockDropOverlay::create();
    m_sceneTree->createDialog = CreateNodeDialog::create(m_nodeTypeCatalog);
    m_sceneTree->createConnection = m_sceneTree->createDialog->events().onConfirmed.connect(
        [this](CreateNodeDialog&, const NodeTypeDescriptor& descriptor, const std::string& parentId) { m_sceneTree->createChild(descriptor, parentId); });
    m_sceneTree->contextMenu = MRPopupMenu::create();
    m_sceneTree->contextMenu->setMenuWidth(220.0f);
    m_sceneTree->contextMenu->addItem(L"Add Child Node...", 1);
    m_sceneTree->contextMenu->addItem(L"Rename...", 2);
    m_sceneTree->contextMenu->addItem(L"Delete", 3);
    m_sceneTree->contextMenuConnection = m_sceneTree->contextMenu->events().onItemSelected.connect([this](MRPopupMenu&, int id, const std::wstring&) {
        if (id == 1)
            m_sceneTree->showCreateDialog(m_sceneTree->contextParentId);
        else if (id == 2)
            m_sceneTree->beginRename(m_sceneTree->contextParentId);
        else if (id == 3)
            m_sceneTree->deleteSelected();
    });
    m_assetsPanel->createAssetDialog = CreateAssetDialog::create();
    m_assetsPanel->assetCreateConnection = m_assetsPanel->createAssetDialog->events().onConfirmed.connect(
        [this](CreateAssetDialog&, const AssetTypeDescriptor& descriptor, const std::string& name) { m_assetsPanel->createAsset(descriptor, name); });
    m_assetsPanel->contextMenu = MRPopupMenu::create();
    m_assetsPanel->contextMenu->addItem(L"Create New Resource...", 1);
    m_assetsPanel->contextMenu->addItem(L"Create Folder", 2);
    m_assetsPanel->contextMenu->addItem(L"Rename", 3);
    m_assetsPanel->contextMenu->addItem(L"Delete", 4);
    m_assetsPanel->contextMenuConnection = m_assetsPanel->contextMenu->events().onItemSelected.connect(
        [this](MRPopupMenu&, int id, const std::wstring&) {
            if (id == 1)
                m_assetsPanel->showCreateDialog();
            else if (id == 2)
                m_assetsPanel->createFolder();
            else if (id == 3)
                m_assetsPanel->renameSelected();
            else if (id == 4)
                m_assetsPanel->deleteSelected();
        });
    m_assetsPanel->nameDialog = RenameNodeDialog::create();
    m_assetsPanel->nameDialogConnection = m_assetsPanel->nameDialog->events().onConfirmed.connect(
        [this](RenameNodeDialog&, const std::string& action, const std::string& name) { m_assetsPanel->applyNameDialog(action, name); });
    m_assetsPanel->view->treeContextConnection = m_assetsPanel->view->treeEvents().onNodeContextMenu.connect(
        [this](MRTree&, int id, const std::wstring&, float x, float y) {
            m_assetsPanel->showContextMenu(m_fileSystem.findById(id), x, y);
        });
    m_assetsPanel->view->setContextMenuCallback([this](const ProjectFileEntry* entry, float x, float y) {
        m_assetsPanel->showContextMenu(entry, x, y);
    });
    m_inspector->assetMenu = MRPopupMenu::create();
    m_inspector->assetMenu->setMenuWidth(320.0f);
    m_inspector->assetMenuConnection = m_inspector->assetMenu->events().onItemSelected.connect([this](MRPopupMenu&, int id, const std::wstring&) {
        const auto iterator = m_inspector->assetMenuIds.find(id);
        if (iterator == m_inspector->assetMenuIds.end())
            return;
        std::string error;
        bool changed = false;
        for (const auto& nodeId : m_inspector->assetEditNodeIds) {
            if (!m_session->setProperty(nodeId, m_inspector->assetEditProperty, iterator->second, false, error)) {
                break;
            }
            changed = true;
        }
        if (changed) {
            m_viewport->syncSelectedRuntimeNodes(false);
            m_viewport->refreshSelectionOverlay();
            m_inspector->refresh();
            setStatus(iterator->second.empty() ? "Cleared texture resource" : "Assigned texture asset " + iterator->second);
        } else if (!error.empty()) {
            setStatus(error);
        }
    });

    m_centerSplit = MRSplitContainer::create();
    m_centerSplit->setOrientation(SplitOrientation::Horizontal);
    m_centerSplit->setFirstMinSize(320.0f);
    m_centerSplit->setSecondMinSize(260.0f);
    m_centerSplit->setHandleWidth(5.0f);

    m_centerSplit->setFirst(m_centerTabs);
    m_centerSplit->setSecond(m_inspector->panel);
    m_mainSplit->setFirst(m_leftTabs);
    m_mainSplit->setSecond(m_centerSplit);
    m_workspaceSplit->setFirst(m_mainSplit);
    m_workspaceSplit->setSecond(m_bottomTabs);
    m_layout->attach(m_dockLayout, m_dockLayoutPath, m_workspaceSplit, m_mainSplit, m_centerSplit, m_leftTabs, m_centerTabs, m_bottomTabs, m_dockDropOverlay);

    m_shellRoot->addChild(m_toolbar->panel);
    m_shellRoot->addChild(m_workspaceSplit);
    m_shellRoot->addChild(m_dockDropOverlay);
    m_shellRoot->addChild(m_sceneTree->createDialog);
    m_shellRoot->addChild(m_assetsPanel->createAssetDialog);
    m_shellRoot->addChild(m_assetsPanel->nameDialog);
    m_sceneTree->createDialog->getTransform()->setSize(m_shellRoot->getTransform()->getSize());
    m_assetsPanel->createAssetDialog->getTransform()->setSize(m_shellRoot->getTransform()->getSize());
    m_assetsPanel->nameDialog->getTransform()->setSize(m_shellRoot->getTransform()->getSize());
    m_viewport->panel->addChild(m_viewport->previewRoot);
    m_window->addChild(m_shellRoot);

    const auto connectSplit = [this](const std::string& id, const std::shared_ptr<MRSplitContainer>& split) {
        m_splitConnections.emplace_back(split->events().onSplitRatioChanged.connect([this, id](MRSplitContainer&, float ratio) {
            if (auto* state = m_dockLayout.findSplit(id))
                state->ratio = ratio;
            applyDockLayout();
        }));
        m_splitConnections.emplace_back(split->events().onDragFinished.connect([this, id](MRSplitContainer&, float ratio) {
            if (auto* state = m_dockLayout.findSplit(id))
                state->ratio = ratio;
            saveDockLayout();
        }));
    };
    connectSplit("workspace", m_workspaceSplit);
    connectSplit("left", m_mainSplit);
    connectSplit("center", m_centerSplit);

    const auto connectTabs = [this](const std::string& id, const std::shared_ptr<MRTabContainer>& tabs) {
        m_tabConnections.emplace_back(tabs->events().onCurrentTabChanged.connect([this, id](MRTabContainer& container, const std::string& active) {
            if (auto* state = m_dockLayout.findTabs(id))
                state->active = active;
            saveDockLayout();
            if (id == "bottom_dock" && active == "build")
                m_output->refresh();
            (void)container;
        }));
        m_tabOrderConnections.emplace_back(tabs->events().onTabOrderChanged.connect([this, id](MRTabContainer& container) {
            if (auto* state = m_dockLayout.findTabs(id)) {
                state->panels.clear();
                for (const auto& tab : container.tabs())
                    state->panels.push_back(tab.id);
            }
            saveDockLayout();
        }));
    };
    connectTabs("left_dock", m_leftTabs);
    connectTabs("center_dock", m_centerTabs);
    connectTabs("bottom_dock", m_bottomTabs);

    m_toolbar->build();
    addLabel(m_sceneTree->panel, "Scene", 8.0f, 6.0f, 220.0f, 28.0f);
    m_sceneTree->createButton = MRButton::create();
    m_sceneTree->createButton->setText(L"+", "default");
    m_sceneTree->createButton->setTextFontSize(16.0f);
    m_sceneTree->createButton->setBackgroundColor(Vector4(0.16f, 0.31f, 0.55f, 1.0f));
    m_sceneTree->createButton->setHoverColor(Vector4(0.25f, 0.48f, 0.76f, 1.0f));
    m_sceneTree->createButton->getTransform()->setPosition(196.0f, 4.0f, 0.0f);
    m_sceneTree->createButton->getTransform()->setSize(34.0f, 28.0f);
    m_sceneTree->createButtonConnection = m_sceneTree->createButton->events().onClicked.connect([this](BaseButton&) { m_sceneTree->showCreateDialog(); });
    m_sceneTree->panel->addChild(m_sceneTree->createButton);
    addLabel(m_viewport->panel, "2D Viewport", 8.0f, 6.0f, 220.0f, 28.0f);
    addLabel(m_inspector->panel, "Inspector", 8.0f, 6.0f, 260.0f, 28.0f);
    addLabel(m_output->panel, "Output / Assets / Build", 8.0f, 2.0f, 360.0f, 22.0f);
    addLabel(m_build->panel, "Build / Run", 8.0f, 2.0f, 260.0f, 22.0f);
    const auto makeLogEdit = [this](const std::shared_ptr<UIWidget>& panel) {
        auto edit = MRTextEdit::create();
        edit->setReadOnly(true);
        edit->setFontSize(15.0f);
        edit->setAutoWrap(false);
        edit->setTextColor(Vector4(0.92f, 0.94f, 0.97f, 1.0f));
        edit->setBackgroundColor(Vector4(0.08f, 0.10f, 0.13f, 1.0f));
        edit->setFocusedBackgroundColor(Vector4(0.09f, 0.12f, 0.16f, 1.0f));
        if (auto interaction = edit->getComponent<Interaction>())
            interaction->setClickEnabled(false);
        m_copyConnections.emplace_back(edit->events().onCopyRequested.connect([this](MRTextEdit&, const std::wstring& selected) {
            if (!m_window || !m_window->getSurface())
                return;
            const auto utf8 = narrow(selected);
            glfwSetClipboardString(static_cast<GLFWwindow*>(m_window->getSurface()), utf8.c_str());
        }));
        panel->addChild(edit);
        return edit;
    };
    m_output->log = makeLogEdit(m_output->panel);
    m_build->log = makeLogEdit(m_build->panel);
    applyDockLayout();
    m_viewport->resizeToPanel();
    m_output->lines = m_outputLines;
    m_output->refresh();
}

void EditorShell::applyDockLayout() {
    m_layout->apply();
}
void EditorShell::syncDockTabs() {
    m_layout->syncTabs();
}
std::shared_ptr<MRTabContainer> EditorShell::tabContainerForId(const std::string& id) const {
    return m_layout->tabContainerForId(id);
}
std::string EditorShell::tabGroupForWidget(const std::shared_ptr<Widget>& widget) const {
    return {};
}
DockDropZone EditorShell::dropZoneAt(float x, float y) const {
    return m_layout->dropZoneAt(x, y);
}
bool EditorShell::handleAssetDrag(const TouchEvent& event) {
    if (event.eventType == TOUCH_EVENT_TYPE_TOUCH && event.button == TOUCH_MOUSE_BUTTON_LEFT) {
        const auto* entry = m_assetsPanel->view ? m_assetsPanel->view->entryForWidget(event.target) : nullptr;
        if (!entry || entry->directory || entry->assetId.empty())
            return false;
        const auto* asset = m_assets.findById(entry->assetId);
        if (!asset || !asset->error.empty())
            return false;
        m_pendingAssetId = asset->assetId;
        m_pendingAssetSource = event.target;
        m_assetDragStartX = event.positionX;
        m_assetDragStartY = event.positionY;
        return false;
    }

    if (event.eventType == TOUCH_EVENT_TYPE_MOVE) {
        if (m_dragDrop.isActive() && m_dragDrop.state().payload.type == "editor/asset") {
            m_dragDrop.update(event.positionX, event.positionY);
            if (m_assetDragPreview)
                m_assetDragPreview->getTransform()->setPosition(event.positionX + 12.0f, event.positionY + 12.0f, 0.0f);
            const auto* asset = m_assets.findById(m_dragDrop.state().payload.id);
            m_inspector->setAssetDropTarget(asset ? m_inspector->assetPropertyAt(event.positionX, event.positionY, *asset) : std::string{});
            return true;
        }
        if (!m_pendingAssetId.empty()) {
            const float dx = event.positionX - m_assetDragStartX;
            const float dy = event.positionY - m_assetDragStartY;
            if (dx * dx + dy * dy >= 36.0f) {
                DragPayload payload;
                payload.type = "editor/asset";
                payload.id = m_pendingAssetId;
                payload.source = m_pendingAssetSource;
                if (!m_dragDrop.begin(payload, event.positionX, event.positionY))
                    return false;
                const auto* asset = m_assets.findById(m_pendingAssetId);
                if (asset && m_shellRoot) {
                    m_assetDragPreview = MRButton::create();
                    m_assetDragPreview->setText(std::wstring(asset->sourcePath.filename().wstring()), "default");
                    m_assetDragPreview->setTextFontSize(13.0f);
                    m_assetDragPreview->setTextAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
                    m_assetDragPreview->setTextColor(Vector4(0.94f, 0.96f, 0.99f, 1.0f));
                    m_assetDragPreview->setBackgroundColor(Vector4(0.08f, 0.12f, 0.18f, 0.94f));
                    m_assetDragPreview->setHoverColor(Vector4(0.08f, 0.12f, 0.18f, 0.94f));
                    m_assetDragPreview->setPressedColor(Vector4(0.08f, 0.12f, 0.18f, 0.94f));
                    m_assetDragPreview->setCornerRadius(3.0f);
                    m_assetDragPreview->setDisplayLayer(10);
                    m_assetDragPreview->getTransform()->setPosition(event.positionX + 12.0f, event.positionY + 12.0f, 0.0f);
                    m_assetDragPreview->getTransform()->setSize(240.0f, 28.0f);
                    if (auto interaction = m_assetDragPreview->getComponent<Interaction>())
                        interaction->setInteractionEnabled(false);
                    m_shellRoot->addChild(m_assetDragPreview);
                }
                m_inspector->setAssetDropTarget(asset ? m_inspector->assetPropertyAt(event.positionX, event.positionY, *asset) : std::string{});
                if (asset)
                    setStatus("Dragging asset " + asset->sourcePath.generic_string());
                return true;
            }
        }
        return false;
    }

    if (event.eventType == TOUCH_EVENT_TYPE_RELEASE) {
        if (m_dragDrop.isActive() && m_dragDrop.state().payload.type == "editor/asset") {
            const DragState state = m_dragDrop.state();
            const auto* asset = m_assets.findById(state.payload.id);
            const std::string property = asset ? m_inspector->assetPropertyAt(event.positionX, event.positionY, *asset) : std::string{};
            m_dragDrop.drop(event.positionX, event.positionY);
            if (m_assetDragPreview && m_shellRoot) {
                m_shellRoot->removeChild(m_assetDragPreview);
                m_assetDragPreview.reset();
            }
            m_inspector->setAssetDropTarget({});
            m_pendingAssetId.clear();
            m_pendingAssetSource.reset();
            if (!asset) {
                setStatus("Dropped asset is no longer available");
            } else if (property.empty()) {
                setStatus("Asset cannot be assigned to this Inspector field");
            } else {
                m_inspector->applyValue(property, asset->assetId);
            }
            return true;
        }
        m_pendingAssetId.clear();
        m_pendingAssetSource.reset();
        if (m_assetDragPreview && m_shellRoot) {
            m_shellRoot->removeChild(m_assetDragPreview);
            m_assetDragPreview.reset();
        }
        m_inspector->setAssetDropTarget({});
    }
    return false;
}

bool EditorShell::handleDockDrag(const TouchEvent& event) {
    return m_layout->handleDrop(event);
}
void EditorShell::completeDockDrop(float x, float y) {
    (void)x;
    (void)y;
}
void EditorShell::saveDockLayout() {
    m_layout->save();
}
void EditorShell::handleFramebufferResize(const Vector2& size) {
    if (!m_shellRoot || size.x <= 0.0f || size.y <= 0.0f)
        return;
    m_shellRoot->getTransform()->setSize(size.x, size.y);
    m_toolbar->panel->getTransform()->setSize(size.x, 40.0f);
    if (m_assetsPanel->createAssetDialog)
        m_assetsPanel->createAssetDialog->getTransform()->setSize(size.x, size.y);
    if (m_assetsPanel->nameDialog)
        m_assetsPanel->nameDialog->getTransform()->setSize(size.x, size.y);
    m_workspaceSplit->getTransform()->setPosition(0.0f, 40.0f, 0.0f);
    m_workspaceSplit->getTransform()->setSize(size.x, std::max(1.0f, size.y - 40.0f));
    applyDockLayout();
    if (m_sceneTree->createDialog)
        m_sceneTree->createDialog->getTransform()->setSize(size.x, size.y);
    m_viewport->resizeToPanel();
    m_output->refresh();
}

void EditorShell::handleInput(std::vector<TouchEvent>& events) {
    m_build->poll();
    for (const auto& event : events) {
        const bool pointerEvent = event.eventType == TOUCH_EVENT_TYPE_TOUCH || event.eventType == TOUCH_EVENT_TYPE_MOVE || event.eventType == TOUCH_EVENT_TYPE_RELEASE ||
                                  event.eventType == TOUCH_EVENT_TYPE_WHEEL;
        if (pointerEvent && m_sceneTree->createDialog && m_sceneTree->createDialog->isOpen()) {
            m_viewport->dragging = false;
            m_viewport->resizing = false;
            m_viewport->resizeHandle = ResizeHandle::None;
            m_viewport->panning = false;
            m_viewport->transformChanged = false;
            continue;
        }
        if (event.eventType == TOUCH_EVENT_TYPE_TOUCH) {
            if (m_sceneTree->renameEdit && !isDescendantOf(event.target, m_sceneTree->renameEdit)) {
                m_sceneTree->commitRename();
                continue;
            }
            if (m_sceneTree->contextMenu && m_sceneTree->contextMenu->isOpen() && !isDescendantOf(event.target, m_sceneTree->contextMenu)) {
                m_sceneTree->contextMenu->hide();
            }
            if (m_inspector->assetMenu && m_inspector->assetMenu->isOpen() && !isDescendantOf(event.target, m_inspector->assetMenu)) {
                m_inspector->assetMenu->hide();
            }
            if (m_assetsPanel->contextMenu && m_assetsPanel->contextMenu->isOpen() && !isDescendantOf(event.target, m_assetsPanel->contextMenu))
                m_assetsPanel->contextMenu->hide();
        }
        if (event.eventType == TOUCH_EVENT_TYPE_KEY_DOWN && event.keyCode == TOUCH_KEY_DELETE && !event.target &&
            (!m_sceneTree->createDialog || !m_sceneTree->createDialog->isOpen()) && !m_sceneTree->renameEdit) {
            const bool fileSystemActive = m_bottomTabs && m_bottomTabs->currentTabId() == "filesystem" &&
                                          (!m_assetsPanel->createAssetDialog || !m_assetsPanel->createAssetDialog->isOpen()) &&
                                          (!m_assetsPanel->nameDialog || !m_assetsPanel->nameDialog->isOpen());
            if (fileSystemActive && m_assetsPanel->view) {
                const auto selected = m_assetsPanel->view->selectedEntries();
                if (!selected.empty()) {
                    std::vector<std::filesystem::path> paths;
                    paths.reserve(selected.size());
                    for (const auto* entry : selected)
                        paths.push_back(entry->relativePath);
                    m_assetsPanel->deletePaths(paths);
                    continue;
                }
            }
            m_sceneTree->deleteSelected();
            continue;
        }
        if (handleAssetDrag(event))
            continue;
        if (m_sceneTree->handleDrag(event))
            continue;
        if (handleDockDrag(event))
            continue;
        const bool activeViewportGesture = m_viewport->dragging || m_viewport->resizing || m_viewport->panning;
        const bool insideViewport = m_viewport->panel && m_viewport->panel->getScreenSpaceAABB().Contains(event.positionX, event.positionY);
        if (pointerEvent && (activeViewportGesture || insideViewport)) {
            m_viewport->handlePointer(event);
        }
    }
}

void EditorShell::handleKey(int key, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (m_assetsPanel->contextMenu && m_assetsPanel->contextMenu->isOpen()) {
            m_assetsPanel->contextMenu->hide();
            return;
        }
        if (m_assetsPanel->createAssetDialog && m_assetsPanel->createAssetDialog->isOpen()) {
            m_assetsPanel->createAssetDialog->hideDialog();
            return;
        }
        if (m_assetsPanel->nameDialog && m_assetsPanel->nameDialog->isOpen()) {
            m_assetsPanel->nameDialog->hideDialog();
            return;
        }
    }
    if (m_sceneTree->createDialog && m_sceneTree->createDialog->isOpen() && key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        m_sceneTree->createDialog->hideDialog();
        return;
    }
    if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
        if (m_engine) {
            m_engine->toggleDebugOverlay();
        }
        return;
    }

    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;
    if (m_sceneTree->renameEdit) {
        if (key == GLFW_KEY_ESCAPE) {
            m_sceneTree->cancelRename();
            return;
        }
        if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
            m_sceneTree->commitRename();
            return;
        }
        TouchKeyCode mappedKey = TOUCH_KEY_UNKNOWN;
        if (key == GLFW_KEY_BACKSPACE)
            mappedKey = TOUCH_KEY_BACKSPACE;
        else if (key == GLFW_KEY_DELETE)
            mappedKey = TOUCH_KEY_DELETE;
        else if (key == GLFW_KEY_LEFT)
            mappedKey = TOUCH_KEY_LEFT;
        else if (key == GLFW_KEY_RIGHT)
            mappedKey = TOUCH_KEY_RIGHT;
        else if (key == GLFW_KEY_HOME)
            mappedKey = TOUCH_KEY_HOME;
        else if (key == GLFW_KEY_END)
            mappedKey = TOUCH_KEY_END;
        else if (key == GLFW_KEY_A)
            mappedKey = TOUCH_KEY_A;
        else if (key == GLFW_KEY_C)
            mappedKey = TOUCH_KEY_C;
        if (mappedKey != TOUCH_KEY_UNKNOWN) {
            TouchEvent event;
            event.eventType = TOUCH_EVENT_TYPE_KEY_DOWN;
            event.deviceType = TOUCH_DEVICE_TYPE_KEYBOARD;
            event.keyCode = mappedKey;
            if ((mods & GLFW_MOD_SHIFT) != 0)
                event.modifiers |= TOUCH_MODIFIER_SHIFT;
            if ((mods & GLFW_MOD_CONTROL) != 0)
                event.modifiers |= TOUCH_MODIFIER_CTRL;
            if ((mods & GLFW_MOD_ALT) != 0)
                event.modifiers |= TOUCH_MODIFIER_ALT;
            m_sceneTree->renameEdit->dispatchTouchEvent(event);
        }
        return;
    }
    if (!m_inspector->legacyEditProperty.empty()) {
        if (key == GLFW_KEY_ENTER) {
            m_inspector->commitLegacyPropertyEdit();
            return;
        }
        if (key == GLFW_KEY_ESCAPE) {
            m_inspector->legacyEditProperty.clear();
            m_inspector->legacyEditValue.clear();
            setStatus("Property edit cancelled");
            return;
        }
        if (key == GLFW_KEY_BACKSPACE && !m_inspector->legacyEditValue.empty()) {
            m_inspector->legacyEditValue.pop_back();
            setStatus("Editing " + m_inspector->legacyEditProperty + ": " + m_inspector->legacyEditValue);
            return;
        }
    }
    char command = 0;
    if (key == GLFW_KEY_S)
        command = 's';
    else if (key == GLFW_KEY_Z)
        command = 'z';
    else if (key == GLFW_KEY_Y)
        command = 'y';
    if (!command || !(mods & GLFW_MOD_CONTROL))
        return;
    std::string error;
    const bool changed =
        command == 's' ? m_session->save(error) : (command == 'z' ? ((mods & GLFW_MOD_SHIFT) ? m_session->redo(error) : m_session->undo(error)) : m_session->redo(error));
    if (changed && command != 's')
        m_viewport->rebuildRuntime();
    setStatus(error.empty() ? "Shortcut applied" : error);
    m_sceneTree->refresh();
    m_inspector->refresh();
}

void EditorShell::handleChar(unsigned int codepoint) {
    if (m_sceneTree->renameEdit && codepoint >= 32 && codepoint <= 0x10FFFF) {
        TouchEvent event;
        event.eventType = TOUCH_EVENT_TYPE_CHARACTER;
        event.deviceType = TOUCH_DEVICE_TYPE_KEYBOARD;
        event.unicodeCodepoint = codepoint;
        m_sceneTree->renameEdit->dispatchTouchEvent(event);
        return;
    }
    m_inspector->appendLegacyCharacter(codepoint);
}

void EditorShell::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (const auto iterator = previousKeyCallbacks().find(window); iterator != previousKeyCallbacks().end() && iterator->second) {
        iterator->second(window, key, scancode, action, mods);
    }
    const auto iterator = shells().find(window);
    if (iterator != shells().end() && iterator->second) {
        iterator->second->handleKey(key, action, mods);
    }
}

void EditorShell::charCallback(GLFWwindow* window, unsigned int codepoint) {
    if (const auto iterator = previousCharCallbacks().find(window); iterator != previousCharCallbacks().end() && iterator->second) {
        iterator->second(window, codepoint);
    }
    const auto iterator = shells().find(window);
    if (iterator != shells().end() && iterator->second)
        iterator->second->handleChar(codepoint);
}

bool EditorShell::initialize(std::string& error) {
    if (!m_project.load(m_projectPath, error))
        return false;
    if (!m_session->load(error))
        return false;
    if (!m_assets.scan(m_projectPath.parent_path(), m_assetRoot, error)) {
        return false;
    }
    if (!m_fileSystem.scan(m_project.projectRoot(), &m_assets, error)) {
        return false;
    }
    buildLayout();
    if (m_window) {
        m_framebufferSizeConnection = m_window->events().onFramebufferSizeChanged.connect([this](const Vector2& size) { handleFramebufferResize(size); });
    }
    m_assetsPanel->importAssets();
    m_sceneTree->refresh();
    m_viewport->rebuildRuntime();
    if (m_window) {
        auto* glfwWindow = static_cast<GLFWwindow*>(m_window->getSurface());
        if (glfwWindow) {
            shells()[glfwWindow] = this;
            previousKeyCallbacks()[glfwWindow] = glfwSetKeyCallback(glfwWindow, keyCallback);
            previousCharCallbacks()[glfwWindow] = glfwSetCharCallback(glfwWindow, charCallback);
        }
    }
    if (m_engine && m_engine->getFrameState() && m_engine->getFrameState()->inputEventsManager) {
        m_inputConnection =
            m_engine->getFrameState()->inputEventsManager->getResolvedInputEventsDispatcher().connect([this](std::vector<TouchEvent>& events) { handleInput(events); });
    }
    setStatus("Ready");
    return true;
}

}  // namespace morrow::editor
