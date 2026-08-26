#include "EditorShell.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <codecvt>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "Engine.h"
#include "base/Interaction.h"
#include "base/TouchEvent.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "elements/MRColor.h"
#include "elements/MRLabel.h"
#include "elements/MRLineEdit.h"
#include "elements/MRPopupMenu.h"
#include "platform/Window.h"
#include "scene/SceneInstantiator.h"
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

bool parseTypedComponents(const std::string& value, const std::string& type, std::vector<std::string>& components) {
    if (value.rfind(type + "(", 0) != 0 || value.empty() || value.back() != ')') {
        return false;
    }
    std::istringstream stream(value.substr(type.size() + 1, value.size() - type.size() - 2));
    std::string component;
    while (std::getline(stream, component, ',')) {
        const auto first = component.find_first_not_of(" \t");
        const auto last = component.find_last_not_of(" \t");
        components.push_back(first == std::string::npos ? std::string{} : component.substr(first, last - first + 1));
    }
    return !components.empty();
}

std::string typedComponents(const std::string& type, const std::vector<std::string>& components) {
    std::ostringstream output;
    output << type << '(';
    for (size_t index = 0; index < components.size(); ++index) {
        if (index > 0)
            output << ", ";
        output << components[index];
    }
    output << ')';
    return output.str();
}

const morrow::editor::InspectorProperty* findInspectorProperty(const std::vector<morrow::editor::InspectorProperty>& properties, const std::string& name) {
    const auto iterator = std::find_if(properties.begin(), properties.end(), [&name](const morrow::editor::InspectorProperty& property) { return property.name == name; });
    return iterator == properties.end() ? nullptr : &*iterator;
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
    m_session(std::make_shared<EditorSession>(m_scenePath)), m_dockLayoutPath(m_projectPath.parent_path() / ".morrow" / "editor.layout") {
    m_selectionChangedConnection = m_events.onSelectionChanged.connect([this](const SelectionState& selection) {
        m_selectedNodeId = selection.nodeIds.empty() ? std::string{} : selection.nodeIds.back();
        refreshSelectionOverlay();
        refreshInspector();
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
    refreshOutput();
}

void EditorShell::setPreviewState(PreviewState state) {
    m_previewState = state;
    const char* label = "Stopped";
    switch (state) {
        case PreviewState::Starting:
            label = "Starting";
            break;
        case PreviewState::Running:
            label = "Running";
            break;
        case PreviewState::Outdated:
            label = "Outdated";
            break;
        case PreviewState::Failed:
            label = "Failed";
            break;
        case PreviewState::Stopped:
            break;
    }
    setStatus(std::string("Preview: ") + label);
}

void EditorShell::stopPreview() {
    if (m_buildFuture.valid() && m_buildFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        m_buildQueue.cancel();
        setStatus("Preview stop requested");
    } else {
        setPreviewState(PreviewState::Stopped);
    }
}

void EditorShell::runImportQueue() {
    std::vector<ImportTaskResult> results;
    std::string error;
    const bool success = m_importQueue.importAll(m_assets, m_projectPath.parent_path(), "windows", results, error);
    size_t imported = 0;
    size_t skipped = 0;
    size_t failed = 0;
    for (const auto& result : results) {
        if (!result.success)
            ++failed;
        else if (result.skipped)
            ++skipped;
        else
            ++imported;
    }
    setStatus("Assets: imported=" + std::to_string(imported) + " unchanged=" + std::to_string(skipped) + " failed=" + std::to_string(failed));
    if (!success && !error.empty())
        setStatus("Import failed: " + error);
    std::string scanError;
    if (!m_assets.scan(m_projectPath.parent_path(), m_assetRoot, scanError)) {
        setStatus("Asset rescan failed: " + scanError);
    }
    if (!m_fileSystem.projectRoot().empty()) {
        if (!m_fileSystem.refresh(scanError)) {
            setStatus("FileSystem refresh failed: " + scanError);
        } else if (m_fileSystemPanel) {
            m_fileSystemPanel->refreshView();
        }
    }
    if (!results.empty()) {
        m_events.onAssetDatabaseChanged.notify(m_assets);
    }
}

void EditorShell::notifySelectionChanged() {
    const auto& selection = m_session->model().selection();
    if (selection.nodeIds == m_lastNotifiedSelection) {
        return;
    }
    m_lastNotifiedSelection = selection.nodeIds;
    m_events.onSelectionChanged.notify(selection);
    refreshSceneTreeSelectionStyles();
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

    m_toolbarPanel = makePanel(0.0f, 0.0f, windowWidth, 40.0f);
    m_sceneTreePanel = makePanel(0.0f, 0.0f, 240.0f, 570.0f, morrow::Math::Vector4(0.13f, 0.15f, 0.19f, 1.0f));
    m_viewportPanel = makePanel(0.0f, 0.0f, 740.0f, 570.0f, morrow::Math::Vector4(0.10f, 0.12f, 0.15f, 1.0f));
    m_inspectorPanel = makePanel(0.0f, 0.0f, 300.0f, 570.0f, morrow::Math::Vector4(0.13f, 0.15f, 0.19f, 1.0f));
    m_statusPanel = makePanel(0.0f, 0.0f, 1280.0f, 140.0f, morrow::Math::Vector4(0.11f, 0.13f, 0.16f, 1.0f));
    m_buildPanel = makePanel(0.0f, 0.0f, 1280.0f, 140.0f, morrow::Math::Vector4(0.11f, 0.13f, 0.16f, 1.0f));
    m_fileSystemPanel = FileSystemPanel::create(m_fileSystem, [this](const std::string& status) { setStatus(status); });
    m_previewRoot = makePanel(0.0f, 32.0f, 740.0f, 538.0f);
    m_previewRoot->setWidgetName("PreviewRoot");

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
    m_leftTabs->addTab("scene_tree", L"Scene", m_sceneTreePanel);
    m_leftTabs->addTab("filesystem", L"FileSystem", m_fileSystemPanel);

    m_centerTabs = MRTabContainer::create();
    m_centerTabs->addTab("viewport", L"Viewport", m_viewportPanel);

    m_bottomTabs = MRTabContainer::create();
    m_bottomTabs->addTab("output", L"Output", m_statusPanel);
    m_bottomTabs->addTab("build", L"Build", m_buildPanel);
    m_dockDropOverlay = DockDropOverlay::create();
    m_createNodeDialog = CreateNodeDialog::create(m_nodeTypeCatalog);
    m_createNodeConnection = m_createNodeDialog->events().onConfirmed.connect(
        [this](CreateNodeDialog&, const NodeTypeDescriptor& descriptor, const std::string& parentId) { createChildNode(descriptor, parentId); });
    m_sceneContextMenu = MRPopupMenu::create();
    m_sceneContextMenu->setMenuWidth(220.0f);
    m_sceneContextMenu->addItem(L"Add Child Node...", 1);
    m_sceneContextMenu->addItem(L"Rename...", 2);
    m_sceneContextMenu->addItem(L"Delete", 3);
    m_sceneContextMenuConnection = m_sceneContextMenu->events().onItemSelected.connect([this](MRPopupMenu&, int id, const std::wstring&) {
        if (id == 1)
            showCreateNodeDialog(m_sceneContextParentId);
        else if (id == 2)
            beginSceneNodeRename(m_sceneContextParentId);
        else if (id == 3)
            deleteSelectedSceneNode();
    });
    m_textureAssetMenu = MRPopupMenu::create();
    m_textureAssetMenu->setMenuWidth(320.0f);
    m_textureAssetMenuConnection = m_textureAssetMenu->events().onItemSelected.connect([this](MRPopupMenu&, int id, const std::wstring&) {
        const auto iterator = m_textureAssetMenuIds.find(id);
        if (iterator == m_textureAssetMenuIds.end())
            return;
        std::string error;
        bool changed = false;
        for (const auto& nodeId : m_textureEditNodeIds) {
            if (!m_session->setProperty(nodeId, m_textureEditProperty, iterator->second, false, error)) {
                break;
            }
            changed = true;
        }
        if (changed) {
            syncSelectedRuntimeNodes(false);
            refreshSelectionOverlay();
            refreshInspector();
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
    m_centerSplit->setSecond(m_inspectorPanel);
    m_mainSplit->setFirst(m_leftTabs);
    m_mainSplit->setSecond(m_centerSplit);
    m_workspaceSplit->setFirst(m_mainSplit);
    m_workspaceSplit->setSecond(m_bottomTabs);

    m_shellRoot->addChild(m_toolbarPanel);
    m_shellRoot->addChild(m_workspaceSplit);
    m_shellRoot->addChild(m_dockDropOverlay);
    m_shellRoot->addChild(m_createNodeDialog);
    m_viewportPanel->addChild(m_previewRoot);
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
                refreshOutput();
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

    addLabel(m_toolbarPanel, "MorrowEditor", 8.0f, 5.0f, 150.0f, 28.0f);
    addButton(m_toolbarPanel, L"Save", 170.0f, 4.0f, 72.0f, 30.0f, [this] {
        std::string error;
        if (m_session->save(error))
            setStatus("Saved");
        else
            setStatus(error);
    });
    addButton(m_toolbarPanel, L"Undo", 248.0f, 4.0f, 72.0f, 30.0f, [this] {
        std::string error;
        if (m_session->undo(error)) {
            rebuildRuntime();
            refreshSceneTree();
            refreshInspector();
        }
        setStatus(error.empty() ? "Undo" : error);
    });
    addButton(m_toolbarPanel, L"Redo", 326.0f, 4.0f, 72.0f, 30.0f, [this] {
        std::string error;
        if (m_session->redo(error)) {
            rebuildRuntime();
            refreshSceneTree();
            refreshInspector();
        }
        setStatus(error.empty() ? "Redo" : error);
    });
    addButton(m_toolbarPanel, L"Configure", 420.0f, 4.0f, 100.0f, 30.0f, [this] { runBuild(BuildTaskKind::Configure); });
    addButton(m_toolbarPanel, L"Build", 526.0f, 4.0f, 80.0f, 30.0f, [this] { runBuild(BuildTaskKind::Build); });
    addButton(m_toolbarPanel, L"Build & Run", 612.0f, 4.0f, 112.0f, 30.0f, [this] { runBuild(BuildTaskKind::BuildAndRun); });
    addButton(m_toolbarPanel, L"Run Last", 730.0f, 4.0f, 92.0f, 30.0f, [this] { runBuild(BuildTaskKind::Run); });
    addButton(m_toolbarPanel, L"Stop", 828.0f, 4.0f, 72.0f, 30.0f, [this] { stopPreview(); });
    addButton(m_toolbarPanel, L"Import", 906.0f, 4.0f, 82.0f, 30.0f, [this] { runImportQueue(); });
    addButton(m_toolbarPanel, L"Assets", 994.0f, 4.0f, 82.0f, 30.0f, [this] { showAssetBrowser(); });
    addLabel(m_sceneTreePanel, "Scene", 8.0f, 6.0f, 220.0f, 28.0f);
    addButton(m_sceneTreePanel, L"+", 196.0f, 4.0f, 34.0f, 28.0f, [this] { showCreateNodeDialog(); });
    addLabel(m_viewportPanel, "2D Viewport", 8.0f, 6.0f, 220.0f, 28.0f);
    addLabel(m_inspectorPanel, "Inspector", 8.0f, 6.0f, 260.0f, 28.0f);
    addLabel(m_statusPanel, "Output / Assets / Build", 8.0f, 2.0f, 360.0f, 22.0f);
    addLabel(m_buildPanel, "Build / Run", 8.0f, 2.0f, 260.0f, 22.0f);
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
    m_outputLogEdit = makeLogEdit(m_statusPanel);
    m_buildLogEdit = makeLogEdit(m_buildPanel);
    applyDockLayout();
    refreshOutput();
}

void EditorShell::applyDockLayout() {
    if (const auto* split = m_dockLayout.findSplit("workspace"))
        m_workspaceSplit->setSplitRatio(split->ratio);
    if (const auto* split = m_dockLayout.findSplit("left"))
        m_mainSplit->setSplitRatio(split->ratio);
    if (const auto* split = m_dockLayout.findSplit("center"))
        m_centerSplit->setSplitRatio(split->ratio);
    const auto applyTabState = [](const DockTabState* state, const std::shared_ptr<MRTabContainer>& tabs) {
        if (!state || !tabs)
            return;
        for (size_t target = 0; target < state->panels.size(); ++target) {
            const auto& desired = state->panels[target];
            size_t current = target;
            while (current < tabs->tabs().size() && tabs->tabs()[current].id != desired) {
                ++current;
            }
            if (current < tabs->tabs().size() && current != target)
                tabs->moveTab(current, target);
        }
        tabs->selectTab(state->active);
    };
    if (const auto* tabs = m_dockLayout.findTabs("left_dock"))
        applyTabState(tabs, m_leftTabs);
    if (const auto* tabs = m_dockLayout.findTabs("center_dock"))
        applyTabState(tabs, m_centerTabs);
    if (const auto* tabs = m_dockLayout.findTabs("bottom_dock"))
        applyTabState(tabs, m_bottomTabs);
    if (m_dockDropOverlay)
        m_dockDropOverlay->setWorkspaceBounds(m_workspaceSplit->getScreenSpaceAABB());
    if (m_createNodeDialog && m_shellRoot)
        m_createNodeDialog->getTransform()->setSize(m_shellRoot->getTransform()->getSize());
    const Vector3 viewportSize = m_viewportPanel->getTransform()->getSize();
    m_viewportWidth = viewportSize.x;
    m_viewportHeight = viewportSize.y;
    m_previewRoot->getTransform()->setSize(viewportSize.x, std::max(1.0f, viewportSize.y - 32.0f));
    if (m_previewCanvas || m_previewGrid)
        refreshViewportGuides();
}

void EditorShell::syncDockTabs() {
    const auto sync = [this](const std::string& id, const std::shared_ptr<MRTabContainer>& tabs) {
        if (auto* state = m_dockLayout.findTabs(id)) {
            state->active = tabs->currentTabId();
            state->panels.clear();
            for (const auto& tab : tabs->tabs())
                state->panels.push_back(tab.id);
        }
    };
    sync("left_dock", m_leftTabs);
    sync("center_dock", m_centerTabs);
    sync("bottom_dock", m_bottomTabs);
}

std::shared_ptr<MRTabContainer> EditorShell::tabContainerForId(const std::string& id) const {
    if (id == "left_dock")
        return m_leftTabs;
    if (id == "center_dock")
        return m_centerTabs;
    if (id == "bottom_dock")
        return m_bottomTabs;
    return nullptr;
}

std::string EditorShell::tabGroupForWidget(const std::shared_ptr<Widget>& widget) const {
    if (m_leftTabs && !m_leftTabs->tabIdForWidget(widget).empty())
        return "left_dock";
    if (m_bottomTabs && !m_bottomTabs->tabIdForWidget(widget).empty())
        return "bottom_dock";
    if (m_centerTabs && !m_centerTabs->tabIdForWidget(widget).empty())
        return "center_dock";
    return {};
}

DockDropZone EditorShell::dropZoneAt(float x, float y) const {
    if (!m_workspaceSplit)
        return DockDropZone::None;
    const Math::Rect bounds = m_workspaceSplit->getScreenSpaceAABB();
    if (!bounds.Contains(x, y))
        return DockDropZone::None;
    const float nx = (x - bounds.Min.x) / std::max(1.0f, bounds.GetWidth());
    const float ny = (y - bounds.Min.y) / std::max(1.0f, bounds.GetHeight());
    if (nx < 0.20f)
        return DockDropZone::Left;
    if (nx > 0.80f)
        return DockDropZone::Right;
    if (ny < 0.20f)
        return DockDropZone::Top;
    if (ny > 0.80f)
        return DockDropZone::Bottom;
    return DockDropZone::Center;
}

bool EditorShell::handleDockDrag(const TouchEvent& event) {
    if (event.eventType == TOUCH_EVENT_TYPE_TOUCH && event.button == TOUCH_MOUSE_BUTTON_LEFT) {
        const auto target = event.target;
        const std::string leftId = m_leftTabs ? m_leftTabs->tabIdForWidget(target) : std::string{};
        const std::string bottomId = m_bottomTabs ? m_bottomTabs->tabIdForWidget(target) : std::string{};
        const std::string centerId = m_centerTabs ? m_centerTabs->tabIdForWidget(target) : std::string{};
        if (!leftId.empty()) {
            m_pendingDockTab = leftId;
            m_pendingDockGroup = "left_dock";
        } else if (!bottomId.empty()) {
            m_pendingDockTab = bottomId;
            m_pendingDockGroup = "bottom_dock";
        } else if (!centerId.empty()) {
            m_pendingDockTab = centerId;
            m_pendingDockGroup = "center_dock";
        } else {
            return false;
        }
        m_pendingDockX = event.positionX;
        m_pendingDockY = event.positionY;
        return false;
    }

    if (event.eventType == TOUCH_EVENT_TYPE_MOVE) {
        if (m_dragDrop.isActive()) {
            m_dragDrop.update(event.positionX, event.positionY);
            if (m_dockDropOverlay) {
                m_dockDropOverlay->setVisible(true);
                m_dockDropOverlay->setZone(dropZoneAt(event.positionX, event.positionY));
            }
            return true;
        }
        if (!m_pendingDockTab.empty()) {
            const float dx = event.positionX - m_pendingDockX;
            const float dy = event.positionY - m_pendingDockY;
            if (dx * dx + dy * dy >= 36.0f) {
                DragPayload payload;
                payload.type = "editor/dock-panel";
                payload.id = m_pendingDockTab;
                payload.source = event.target;
                m_dragDrop.begin(payload, event.positionX, event.positionY);
                if (m_dockDropOverlay) {
                    m_dockDropOverlay->setVisible(true);
                    m_dockDropOverlay->setZone(dropZoneAt(event.positionX, event.positionY));
                }
                return true;
            }
        }
        return false;
    }

    if (event.eventType == TOUCH_EVENT_TYPE_RELEASE) {
        if (m_dragDrop.isActive()) {
            completeDockDrop(event.positionX, event.positionY);
            m_pendingDockTab.clear();
            m_pendingDockGroup.clear();
            return true;
        }
        m_pendingDockTab.clear();
        m_pendingDockGroup.clear();
    }
    return false;
}

void EditorShell::completeDockDrop(float x, float y) {
    const auto state = m_dragDrop.state();
    const std::string panelId = state.payload.id;
    const DockDropZone zone = dropZoneAt(x, y);
    if (m_dockDropOverlay) {
        m_dockDropOverlay->setVisible(false);
        m_dockDropOverlay->setZone(DockDropZone::None);
    }
    m_dragDrop.drop(x, y);
    if (zone == DockDropZone::None)
        return;

    std::shared_ptr<MRTabContainer> targetTabs;
    if (zone == DockDropZone::Bottom) {
        targetTabs = m_bottomTabs;
    } else if (zone == DockDropZone::Left) {
        targetTabs = m_leftTabs;
    } else if (zone == DockDropZone::Right || zone == DockDropZone::Top) {
        targetTabs = m_centerTabs;
    } else {
        const Math::Rect leftBounds = m_leftTabs->getScreenSpaceAABB();
        const Math::Rect centerBounds = m_centerTabs->getScreenSpaceAABB();
        const Math::Rect bottomBounds = m_bottomTabs->getScreenSpaceAABB();
        targetTabs = bottomBounds.Contains(x, y) ? m_bottomTabs : (leftBounds.Contains(x, y) ? m_leftTabs : centerBounds.Contains(x, y) ? m_centerTabs : m_leftTabs);
    }
    if (!targetTabs)
        return;

    std::shared_ptr<MRTabContainer> sourceTabs = m_pendingDockGroup == "bottom_dock" ? m_bottomTabs : (m_pendingDockGroup == "center_dock" ? m_centerTabs : m_leftTabs);
    MRTabContainer::DetachedTab detached;
    if (!sourceTabs->detachTab(panelId, detached))
        return;
    if (!targetTabs->addTab(detached.id, detached.title, detached.content)) {
        sourceTabs->addTab(detached.id, detached.title, detached.content);
        return;
    }
    targetTabs->selectTab(detached.id);
    syncDockTabs();
    saveDockLayout();
}

void EditorShell::saveDockLayout() {
    std::string error;
    if (!m_dockLayout.save(m_dockLayoutPath, error))
        setStatus(error);
}

void EditorShell::handleFramebufferResize(const Vector2& size) {
    if (!m_shellRoot || size.x <= 0.0f || size.y <= 0.0f)
        return;
    m_shellRoot->getTransform()->setSize(size.x, size.y);
    m_toolbarPanel->getTransform()->setSize(size.x, 40.0f);
    m_workspaceSplit->getTransform()->setPosition(0.0f, 40.0f, 0.0f);
    m_workspaceSplit->getTransform()->setSize(size.x, std::max(1.0f, size.y - 40.0f));
    applyDockLayout();
    refreshOutput();
}

void EditorShell::refreshOutput() {
    if (!m_statusPanel || !m_buildPanel)
        return;
    std::wstring text;
    for (size_t index = 0; index < m_outputLines.size(); ++index) {
        if (index > 0)
            text.push_back(L'\n');
        text += wide(m_outputLines[index]);
    }
    const auto updateLog = [&text](const std::shared_ptr<MRTextEdit>& edit, const std::shared_ptr<UIWidget>& panel) {
        if (!edit || !panel)
            return;
        const Vector3 panelSize = panel->getTransform()->getSize();
        edit->getTransform()->setPosition(6.0f, 25.0f, 0.0f);
        edit->getTransform()->setSize(std::max(1.0f, panelSize.x - 12.0f), std::max(1.0f, panelSize.y - 31.0f));
        if (edit->getText() != text)
            edit->setText(text);
    };
    updateLog(m_outputLogEdit, m_statusPanel);
    updateLog(m_buildLogEdit, m_buildPanel);
}

void EditorShell::cancelSceneNodeRename() {
    if (!m_sceneRenameEdit)
        return;
    m_sceneRenameEdit->onFocusChanged(false);
    if (m_sceneTreePanel)
        m_sceneTreePanel->removeChild(m_sceneRenameEdit);
    for (auto& row : m_sceneTreeRows) {
        if (row.nodeId == m_sceneRenameNodeId && row.button) {
            row.button->setVisible(true);
            break;
        }
    }
    m_sceneRenameEdit.reset();
    m_sceneRenameNodeId.clear();
}

void EditorShell::runBuild(BuildTaskKind kind) {
    if (m_buildFuture.valid() && m_buildFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        setStatus("A build process is already running");
        return;
    }
    if ((kind == BuildTaskKind::BuildAndRun || kind == BuildTaskKind::Run) && m_session->isDirty()) {
        std::string saveError;
        if (!m_session->save(saveError)) {
            setStatus("Cannot run unsaved scene: " + saveError);
            return;
        }
        setStatus("Scene saved for runtime");
    }
    const auto projectRoot = m_project.pathValue("cmake_root", m_project.projectRoot());
    const auto buildRoot = m_project.pathValue("build_root", "build");
    const auto target = m_project.value("preview_target", "MorrowEditor");
    const auto generator = m_project.value("build_generator", "MinGW Makefiles");
    const std::vector<std::string> runtimeArguments = {
        "--project",
        m_projectPath.string(),
        "--scene",
        m_scenePath.string(),
    };
    const auto configuredExecutable = buildRoot / (target + ".exe");
    if (m_lastSuccessfulExecutable.empty()) {
        std::error_code searchError;
        if (std::filesystem::exists(configuredExecutable))
            m_lastSuccessfulExecutable = configuredExecutable;
        else if (std::filesystem::exists(buildRoot)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(buildRoot, searchError)) {
                if (!searchError && entry.is_regular_file() && entry.path().filename() == target + ".exe") {
                    m_lastSuccessfulExecutable = entry.path();
                    break;
                }
            }
        }
    }
    if (kind == BuildTaskKind::Run && !m_lastSuccessfulExecutable.empty() && std::filesystem::exists(m_lastSuccessfulExecutable))
        setStatus("Run last successful started");
    else if (kind == BuildTaskKind::Run)
        setStatus("No successful preview executable; using configured target");
    else
        setStatus(kind == BuildTaskKind::Configure ? "Configure started" : (kind == BuildTaskKind::Build ? "Build started" : "Build & Run started"));
    m_pendingBuildKind = kind;
    if (kind == BuildTaskKind::Run || kind == BuildTaskKind::BuildAndRun)
        setPreviewState(PreviewState::Starting);
    m_buildFuture = std::async(std::launch::async, [this, kind, projectRoot, buildRoot, target, generator, runtimeArguments] {
        if (kind == BuildTaskKind::Configure)
            return m_buildQueue.configure(projectRoot, buildRoot, generator);
        if (kind == BuildTaskKind::Build)
            return m_buildQueue.build(projectRoot, buildRoot, target, generator);
        if (kind == BuildTaskKind::BuildAndRun)
            return m_buildQueue.buildAndRun(projectRoot, buildRoot, target, generator, runtimeArguments);
        auto executable = m_lastSuccessfulExecutable.empty() ? buildRoot / (target + ".exe") : m_lastSuccessfulExecutable;
        return m_buildQueue.runTarget(executable, executable.parent_path(), runtimeArguments);
    });
}

void EditorShell::pollBuild() {
    bool outputChanged = false;
    for (const auto& chunk : m_buildQueue.drainOutput()) {
        auto& remainder = chunk.stderrStream ? m_stderrRemainder : m_stdoutRemainder;
        remainder += chunk.text;
        size_t newline = 0;
        while ((newline = remainder.find('\n')) != std::string::npos) {
            auto line = remainder.substr(0, newline);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (!line.empty())
                m_outputLines.push_back(std::string(chunk.stderrStream ? "[stderr] " : "[stdout] ") + line);
            remainder.erase(0, newline + 1);
            outputChanged = true;
        }
    }
    while (m_outputLines.size() > 6) {
        m_outputLines.erase(m_outputLines.begin());
        outputChanged = true;
    }
    if (outputChanged)
        refreshOutput();

    if (!m_buildFuture.valid())
        return;
    if (m_buildQueue.state() == BuildProcessState::Running && (m_pendingBuildKind == BuildTaskKind::Run || m_pendingBuildKind == BuildTaskKind::BuildAndRun) &&
        m_previewState == PreviewState::Starting) {
        m_previewState = PreviewState::Running;
    }
    if (m_buildFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        return;
    const auto result = m_buildFuture.get();
    appendBuildResult(result);
    if (result.success && (m_pendingBuildKind == BuildTaskKind::Build || m_pendingBuildKind == BuildTaskKind::BuildAndRun)) {
        const auto buildRoot = m_project.pathValue("build_root", "build");
        const auto target = m_project.value("preview_target", "MorrowEditor");
        m_lastSuccessfulExecutable = buildRoot / (target + ".exe");
        if (!std::filesystem::exists(m_lastSuccessfulExecutable)) {
            std::error_code searchError;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(buildRoot, searchError)) {
                if (!searchError && entry.is_regular_file() && entry.path().filename() == target + ".exe") {
                    m_lastSuccessfulExecutable = entry.path();
                    break;
                }
            }
        }
        setPreviewState(m_pendingBuildKind == BuildTaskKind::BuildAndRun ? PreviewState::Stopped : PreviewState::Outdated);
    } else if (m_pendingBuildKind == BuildTaskKind::Run || m_pendingBuildKind == BuildTaskKind::BuildAndRun) {
        setPreviewState(result.cancelled ? PreviewState::Stopped : (result.success ? PreviewState::Stopped : PreviewState::Failed));
    }
    setStatus(result.cancelled ? "Process cancelled"
                               : (result.success ? "Process succeeded, exit=" + std::to_string(result.exitCode) : "Process failed, exit=" + std::to_string(result.exitCode)));
}

void EditorShell::appendBuildResult(const BuildTaskResult& result) {
    std::istringstream stdoutStream(result.stdoutText);
    std::string line;
    while (std::getline(stdoutStream, line))
        if (!line.empty())
            m_outputLines.push_back("[stdout] " + line);
    std::istringstream stderrStream(result.stderrText);
    while (std::getline(stderrStream, line))
        if (!line.empty())
            m_outputLines.push_back("[stderr] " + line);
    for (const auto& diagnostic : result.diagnostics) {
        m_outputLines.push_back(std::string(diagnostic.error ? "[error] " : "[warning] ") + diagnostic.file.string() + ":" + std::to_string(diagnostic.line) + ":" +
                                std::to_string(diagnostic.column) + " " + diagnostic.message);
    }
    while (m_outputLines.size() > 6)
        m_outputLines.erase(m_outputLines.begin());
    refreshOutput();
}

void EditorShell::showAssetBrowser() {
    std::string error;
    if (!m_fileSystem.refresh(error)) {
        setStatus("FileSystem refresh failed: " + error);
        return;
    }
    if (m_fileSystemPanel)
        m_fileSystemPanel->refreshView();
    setStatus("FileSystem: " + std::to_string(m_fileSystem.entries().size()) + " entries, " + std::to_string(m_assets.assets().size()) + " assets");
}

void EditorShell::refreshSceneTree() {
    if (!m_sceneTreePanel)
        return;
    cancelSceneNodeRename();
    while (m_sceneTreePanel->m_children.size() > 2) {
        m_sceneTreePanel->m_children.pop_back();
    }
    m_sceneTreeContextConnections.clear();
    m_sceneTreeRows.clear();
    float y = 38.0f;
    const float panelWidth = std::max(80.0f, m_sceneTreePanel->getTransform()->getSize().x);
    for (const auto& item : m_session->model().buildSceneTree()) {
        const auto id = item.id;
        auto button = addButton(m_sceneTreePanel, std::wstring(item.depth * 2, L' ') + wide(item.name), 8.0f + item.depth * 12.0f, y,
                                std::max(40.0f, panelWidth - 16.0f - item.depth * 12.0f), 30.0f, [this, id] {
                                    std::string error;
                                    if (m_session->selectNode(id, false, error)) {
                                        notifySelectionChanged();
                                        setStatus("Selected " + id);
                                    } else
                                        setStatus(error);
                                });
        button->setWidgetName("SceneTree_" + item.id);
        button->setTextAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
        m_sceneTreeRows.push_back({item.id, button});
        if (auto interaction = button->getComponent<Interaction>()) {
            const auto parentId = item.id;
            m_sceneTreeContextConnections.emplace_back(interaction->addEventListener(
                TOUCH_EVENT_TYPE_TOUCH,
                [this, parentId](TouchEvent& event) {
                    if (event.button != TOUCH_MOUSE_BUTTON_RIGHT)
                        return;
                    std::string error;
                    if (m_session->selectNode(parentId, false, error))
                        notifySelectionChanged();
                    m_sceneContextParentId = parentId;
                    m_sceneContextMenu->attachTo(m_shellRoot);
                    m_sceneContextMenu->popup(event.positionX, event.positionY);
                },
                20));
        }
        y += 32.0f;
    }
    refreshSceneTreeSelectionStyles();
}

void EditorShell::refreshSceneTreeSelectionStyles() {
    const auto& selected = m_session->model().selection().nodeIds;
    for (auto& row : m_sceneTreeRows) {
        if (!row.button)
            continue;
        const bool isSelected = std::find(selected.begin(), selected.end(), row.nodeId) != selected.end();
        const bool isDropTarget = m_sceneDragging && row.nodeId == m_sceneDropTarget;
        if (isDropTarget) {
            row.button->setTextColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
            row.button->setBackgroundColor(Vector4(0.15f, 0.52f, 0.38f, 1.0f));
            row.button->setHoverColor(Vector4(0.20f, 0.64f, 0.46f, 1.0f));
            row.button->setPressedColor(Vector4(0.11f, 0.42f, 0.30f, 1.0f));
        } else if (isSelected) {
            row.button->setTextColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
            row.button->setBackgroundColor(Vector4(0.16f, 0.34f, 0.62f, 1.0f));
            row.button->setHoverColor(Vector4(0.22f, 0.44f, 0.74f, 1.0f));
            row.button->setPressedColor(Vector4(0.11f, 0.27f, 0.52f, 1.0f));
        } else {
            row.button->setTextColor(Vector4(0.82f, 0.86f, 0.92f, 1.0f));
            row.button->setBackgroundColor(Vector4(0.13f, 0.15f, 0.19f, 1.0f));
            row.button->setHoverColor(Vector4(0.20f, 0.25f, 0.33f, 1.0f));
            row.button->setPressedColor(Vector4(0.10f, 0.22f, 0.42f, 1.0f));
        }
    }
}

std::string EditorShell::sceneTreeNodeForWidget(const std::shared_ptr<Widget>& widget) const {
    if (!widget)
        return {};
    for (const auto& row : m_sceneTreeRows) {
        if (row.button == widget)
            return row.nodeId;
    }
    return {};
}

std::string EditorShell::sceneTreeNodeAt(float x, float y) const {
    for (const auto& row : m_sceneTreeRows) {
        if (row.button && row.button->getScreenSpaceAABB().Contains(x, y)) {
            return row.nodeId;
        }
    }
    return {};
}

bool EditorShell::handleSceneTreeDrag(const TouchEvent& event) {
    if (event.eventType == TOUCH_EVENT_TYPE_TOUCH && event.button == TOUCH_MOUSE_BUTTON_LEFT) {
        const std::string nodeId = sceneTreeNodeForWidget(event.target);
        if (nodeId.empty())
            return false;
        m_pendingSceneDragNode = nodeId;
        m_sceneDragStartX = event.positionX;
        m_sceneDragStartY = event.positionY;
        return false;
    }

    if (event.eventType == TOUCH_EVENT_TYPE_MOVE) {
        if (!m_sceneDragging && !m_pendingSceneDragNode.empty()) {
            const float dx = event.positionX - m_sceneDragStartX;
            const float dy = event.positionY - m_sceneDragStartY;
            if (dx * dx + dy * dy >= 36.0f) {
                m_sceneDragging = true;
                m_sceneDragNode = m_pendingSceneDragNode;
                std::string error;
                if (m_session->selectNode(m_sceneDragNode, false, error)) {
                    notifySelectionChanged();
                }
            }
        }
        if (!m_sceneDragging)
            return false;
        std::string target = sceneTreeNodeAt(event.positionX, event.positionY);
        if (target == m_sceneDragNode)
            target.clear();
        if (m_sceneDropTarget != target) {
            m_sceneDropTarget = std::move(target);
            refreshSceneTreeSelectionStyles();
        }
        return true;
    }

    if (event.eventType == TOUCH_EVENT_TYPE_RELEASE) {
        m_pendingSceneDragNode.clear();
        if (!m_sceneDragging)
            return false;
        const std::string dragged = m_sceneDragNode;
        const std::string target = m_sceneDropTarget;
        m_sceneDragging = false;
        m_sceneDragNode.clear();
        m_sceneDropTarget.clear();
        refreshSceneTreeSelectionStyles();
        if (target.empty())
            return true;
        const auto* node = m_session->document().findNode(dragged);
        if (node && node->parentId == target) {
            setStatus("Node already has the requested parent");
            return true;
        }
        std::string error;
        if (!m_session->reparentNode(dragged, target, error)) {
            setStatus("Cannot reparent node: " + error);
            return true;
        }
        refreshSceneTree();
        rebuildRuntime();
        refreshInspector();
        setStatus("Reparented " + dragged + " under " + target);
        return true;
    }
    return false;
}

void EditorShell::deleteSelectedSceneNode() {
    const auto selected = m_session->model().selection().nodeIds;
    if (selected.empty()) {
        setStatus("Select a scene node to delete");
        return;
    }
    const std::string nodeId = selected.back();
    const auto* node = m_session->document().findNode(nodeId);
    if (!node) {
        setStatus("Selected scene node no longer exists");
        return;
    }
    if (node->parentId.empty()) {
        setStatus("The scene root node cannot be deleted");
        return;
    }
    std::string error;
    if (!m_session->deleteNode(nodeId, error)) {
        setStatus("Failed to delete node: " + error);
        return;
    }
    m_session->model().clearSelection();
    m_selectedNodeId.clear();
    notifySelectionChanged();
    refreshSceneTree();
    rebuildRuntime();
    refreshInspector();
    setStatus("Deleted node " + nodeId);
}

void EditorShell::beginSceneNodeRename(const std::string& nodeId) {
    std::string resolved = nodeId;
    if (resolved.empty() && !m_session->model().selection().nodeIds.empty()) {
        resolved = m_session->model().selection().nodeIds.back();
    }
    const auto* node = m_session->document().findNode(resolved);
    if (!node) {
        setStatus("Select a scene node to rename");
        return;
    }
    cancelSceneNodeRename();
    const auto row = std::find_if(m_sceneTreeRows.begin(), m_sceneTreeRows.end(), [&resolved](const SceneTreeRow& candidate) { return candidate.nodeId == resolved; });
    if (row == m_sceneTreeRows.end() || !row->button) {
        setStatus("Scene node row is not available");
        return;
    }

    const Vector3 position = row->button->getTransform()->getPosition();
    const Vector3 size = row->button->getTransform()->getSize();
    row->button->setVisible(false);
    m_sceneRenameNodeId = resolved;
    m_sceneRenameEdit = MRLineEdit::create();
    m_sceneRenameEdit->setText(wide(node->name));
    m_sceneRenameEdit->setFontSize(16.0f);
    m_sceneRenameEdit->setTextColor(Vector4(0.98f, 0.99f, 1.0f, 1.0f));
    m_sceneRenameEdit->setBackgroundColor(Vector4(0.08f, 0.18f, 0.34f, 1.0f));
    m_sceneRenameEdit->setFocusedBackgroundColor(Vector4(0.10f, 0.24f, 0.46f, 1.0f));
    if (auto interaction = m_sceneRenameEdit->getComponent<Interaction>()) {
        interaction->setKeyboardFocusable(false);
    }
    m_sceneRenameEdit->getTransform()->setPosition(position);
    m_sceneRenameEdit->getTransform()->setSize(size);
    m_sceneTreePanel->addChild(m_sceneRenameEdit);
    m_sceneRenameEdit->onFocusChanged(true);
    m_sceneRenameEdit->selectAll();
}

void EditorShell::commitSceneNodeRename() {
    if (!m_sceneRenameEdit || m_sceneRenameNodeId.empty())
        return;
    const std::string nodeId = m_sceneRenameNodeId;
    const std::string name = narrow(m_sceneRenameEdit->getText());
    if (name.empty()) {
        setStatus("Node name cannot be empty");
        return;
    }
    renameSceneNode(nodeId, name);
}

void EditorShell::renameSceneNode(const std::string& nodeId, const std::string& name) {
    std::string error;
    if (!m_session->renameNode(nodeId, name, error)) {
        setStatus("Failed to rename node: " + error);
        return;
    }
    refreshSceneTree();
    syncRuntimeNode(nodeId, false);
    refreshSelectionOverlay();
    refreshInspector();
    setStatus("Renamed node " + nodeId + " to " + name);
}

void EditorShell::showCreateNodeDialog(const std::string& parentId) {
    std::string resolvedParent = parentId;
    if (resolvedParent.empty() && !m_session->model().selection().nodeIds.empty()) {
        resolvedParent = m_session->model().selection().nodeIds.back();
    }
    if (resolvedParent.empty()) {
        for (const auto& node : m_session->document().nodes()) {
            if (node.parentId.empty()) {
                resolvedParent = node.id;
                break;
            }
        }
    }
    const auto* parent = m_session->document().findNode(resolvedParent);
    if (!parent) {
        setStatus("Select a valid parent node first");
        return;
    }
    m_createNodeDialog->showForParent(parent->id, parent->name);
}

void EditorShell::createChildNode(const NodeTypeDescriptor& descriptor, const std::string& parentId) {
    SceneNodeRecord node = m_nodeTypeCatalog.createNode(descriptor, parentId, m_session->document());
    const std::string nodeId = node.id;
    const std::string nodeName = node.name;
    std::string error;
    if (!m_session->addNode(std::move(node), error)) {
        setStatus("Failed to create node: " + error);
        return;
    }
    if (!m_session->selectNode(nodeId, false, error)) {
        setStatus("Node created, but selection failed: " + error);
    } else {
        notifySelectionChanged();
    }
    refreshSceneTree();
    rebuildRuntime();
    refreshInspector();
    setStatus("Created " + descriptor.displayName + " '" + nodeName + "' under " + parentId);
}

void EditorShell::refreshInspector(bool force) {
    if (!m_inspectorPanel)
        return;
    auto properties = m_session->model().inspectSelected();
    std::ostringstream signature;
    for (const auto& nodeId : m_session->model().selection().nodeIds) {
        signature << "node:" << nodeId << '\n';
    }
    for (const auto& property : properties) {
        signature << property.name << '\x1f' << property.type << '\x1f' << (property.mixed ? '1' : '0') << '\n';
    }
    const std::string schemaSignature = signature.str();
    if (!force && schemaSignature == m_lastInspectorSchemaSignature && !m_inspectorBindings.empty()) {
        updateInspectorValues(properties);
        return;
    }
    m_lastInspectorSchemaSignature = schemaSignature;

    while (m_inspectorPanel->m_children.size() > 1) {
        m_inspectorPanel->m_children.pop_back();
    }
    m_inspectorEditConnections.clear();
    m_inspectorBindings.clear();
    if (properties.empty())
        return;

    const float panelWidth = std::max(180.0f, m_inspectorPanel->getTransform()->getSize().x);
    const float fieldWidth = panelWidth - 16.0f;
    float y = 38.0f;
    const std::vector<std::string> priority = {"position", "rotation", "scale", "size", "visible", "display_layer", "texture_asset"};
    std::stable_sort(properties.begin(), properties.end(), [&priority](const InspectorProperty& left, const InspectorProperty& right) {
        const auto order = [&priority](const std::string& name) {
            const auto iterator = std::find(priority.begin(), priority.end(), name);
            return iterator == priority.end() ? priority.size() : static_cast<size_t>(std::distance(priority.begin(), iterator));
        };
        return order(left.name) < order(right.name);
    });

    addLabel(m_inspectorPanel, "Transform / Properties", 8.0f, y, fieldWidth, 24.0f);
    y += 28.0f;

    for (const auto& property : properties) {
        addLabel(m_inspectorPanel, property.name, 8.0f, y, fieldWidth, 22.0f);
        y += 24.0f;
        if (property.type == "bool" && !property.mixed) {
            auto button = addButton(m_inspectorPanel, property.value == "true" ? L"[x] true" : L"[ ] false", 8.0f, y, fieldWidth, 30.0f, [this, propertyName = property.name] {
                const auto current = m_session->model().inspectSelected();
                const auto* value = findInspectorProperty(current, propertyName);
                if (!value)
                    return;
                applyInspectorValue(propertyName, value->value == "true" ? "false" : "true");
            });
            m_inspectorBindings[property.name] = {property.name, property.type, {}, button};
        } else if (property.type == "TextureAsset") {
            std::string display = "<empty>";
            if (!property.value.empty()) {
                display = property.value;
                if (const auto* asset = m_assets.findById(property.value)) {
                    display = asset->sourcePath.generic_string();
                }
            }
            auto button = MRButton::create();
            button->setText(std::wstring(display.begin(), display.end()), "default");
            button->setTextFontSize(14.0f);
            button->setBackgroundColor(Vector4(0.075f, 0.085f, 0.105f, 1.0f));
            button->setHoverColor(Vector4(0.14f, 0.20f, 0.29f, 1.0f));
            button->setTextColor(Vector4(0.86f, 0.89f, 0.94f, 1.0f));
            button->getTransform()->setPosition(8.0f, y, 0.0f);
            button->getTransform()->setSize(fieldWidth, 30.0f);
            m_buttonConnections.emplace_back(button->events().onClicked.connect([this, property](BaseButton& source) {
                m_textureAssetMenu->clear();
                m_textureAssetMenuIds.clear();
                m_textureAssetMenu->addItem(L"<empty>", 0);
                m_textureAssetMenuIds[0] = "";
                int itemId = 1;
                for (const auto& asset : m_assets.assets()) {
                    if (asset.type != "Texture" || !asset.error.empty())
                        continue;
                    const std::string pathText = asset.sourcePath.generic_string();
                    m_textureAssetMenu->addItem(std::wstring(pathText.begin(), pathText.end()), itemId);
                    m_textureAssetMenuIds[itemId] = asset.assetId;
                    ++itemId;
                }
                m_textureEditProperty = property.name;
                m_textureEditNodeIds = m_session->model().selection().nodeIds;
                m_textureAssetMenu->attachTo(m_shellRoot);
                m_textureAssetMenu->popupBelow(source.getScreenSpaceAABB());
            }));
            m_inspectorPanel->addChild(button);
            m_inspectorBindings[property.name] = {property.name, property.type, {}, button};
        } else if ((property.type == "Vector2" || property.type == "Vector3") && !property.mixed) {
            std::vector<std::string> components;
            if (!parseTypedComponents(property.value, property.type, components)) {
                components.assign(property.type == "Vector2" ? 2 : 3, "0.0");
            }
            const float width = (fieldWidth - 6.0f * static_cast<float>(components.size() - 1)) / static_cast<float>(components.size());
            for (size_t index = 0; index < components.size(); ++index) {
                auto edit = MRLineEdit::create();
                edit->setFontSize(14.0f);
                edit->setText(std::wstring(components[index].begin(), components[index].end()));
                edit->setBackgroundColor(Vector4(0.075f, 0.085f, 0.105f, 1.0f));
                edit->setFocusedBackgroundColor(Vector4(0.10f, 0.13f, 0.18f, 1.0f));
                edit->setTextColor(index == 0 ? Vector4(0.82f, 0.38f, 0.43f, 1.0f) : (index == 1 ? Vector4(0.55f, 0.78f, 0.32f, 1.0f) : Vector4(0.35f, 0.62f, 0.88f, 1.0f)));
                edit->getTransform()->setPosition(8.0f + static_cast<float>(index) * (width + 6.0f), y, 0.0f);
                edit->getTransform()->setSize(width, 30.0f);
                m_inspectorEditConnections.emplace_back(
                    edit->events().onSubmitted.connect([this, propertyName = property.name, propertyType = property.type, index](MRTextEdit&, const std::wstring& text) {
                        const auto current = m_session->model().inspectSelected();
                        const auto* currentProperty = findInspectorProperty(current, propertyName);
                        if (!currentProperty)
                            return;
                        std::vector<std::string> updated;
                        if (!parseTypedComponents(currentProperty->value, propertyType, updated) || index >= updated.size()) {
                            setStatus("Current vector value is invalid");
                            return;
                        }
                        updated[index] = std::string(text.begin(), text.end());
                        try {
                            size_t consumed = 0;
                            std::stof(updated[index], &consumed);
                            if (consumed != updated[index].size())
                                throw std::invalid_argument("number");
                        } catch (...) {
                            setStatus("Vector component must be numeric");
                            return;
                        }
                        const std::string value = typedComponents(propertyType, updated);
                        m_engine->mainThreadDispatcher().post([this, propertyName, value] { applyInspectorValue(propertyName, value); });
                    }));
                m_inspectorPanel->addChild(edit);
                m_inspectorBindings[property.name].property = property.name;
                m_inspectorBindings[property.name].type = property.type;
                m_inspectorBindings[property.name].edits.push_back(edit);
            }
        } else {
            auto edit = MRLineEdit::create();
            edit->setFontSize(14.0f);
            const std::string initial = property.mixed ? std::string{} : property.value;
            edit->setText(std::wstring(initial.begin(), initial.end()));
            if (property.mixed)
                edit->setPlaceholder(L"<mixed>");
            edit->setBackgroundColor(Vector4(0.075f, 0.085f, 0.105f, 1.0f));
            edit->setFocusedBackgroundColor(Vector4(0.10f, 0.13f, 0.18f, 1.0f));
            edit->setTextColor(Vector4(0.88f, 0.90f, 0.94f, 1.0f));
            edit->getTransform()->setPosition(8.0f, y, 0.0f);
            edit->getTransform()->setSize(fieldWidth, 30.0f);
            m_inspectorEditConnections.emplace_back(edit->events().onSubmitted.connect([this, property](MRTextEdit&, const std::wstring& text) {
                const std::string value(text.begin(), text.end());
                if (property.type == "number") {
                    try {
                        size_t consumed = 0;
                        std::stof(value, &consumed);
                        if (consumed != value.size())
                            throw std::invalid_argument("number");
                    } catch (...) {
                        setStatus("Property must be numeric");
                        return;
                    }
                }
                m_engine->mainThreadDispatcher().post([this, propertyName = property.name, value] { applyInspectorValue(propertyName, value); });
            }));
            m_inspectorPanel->addChild(edit);
            m_inspectorBindings[property.name] = {property.name, property.type, {edit}, {}};
        }
        y += 36.0f;
    }
}

void EditorShell::updateInspectorValues(const std::vector<InspectorProperty>& properties) {
    for (auto& [name, binding] : m_inspectorBindings) {
        const auto* property = findInspectorProperty(properties, name);
        if (!property)
            continue;
        if (binding.type == "bool" && binding.button) {
            binding.button->setText(property->value == "true" ? L"[x] true" : L"[ ] false", "default");
            continue;
        }
        if (binding.type == "TextureAsset" && binding.button) {
            std::string display = "<empty>";
            if (!property->value.empty()) {
                display = property->value;
                if (const auto* asset = m_assets.findById(property->value)) {
                    display = asset->sourcePath.generic_string();
                }
            }
            binding.button->setText(std::wstring(display.begin(), display.end()), "default");
            continue;
        }
        if (binding.type == "Vector2" || binding.type == "Vector3") {
            std::vector<std::string> components;
            if (!parseTypedComponents(property->value, binding.type, components)) {
                continue;
            }
            for (size_t index = 0; index < binding.edits.size() && index < components.size(); ++index) {
                binding.edits[index]->setText(std::wstring(components[index].begin(), components[index].end()));
            }
            continue;
        }
        if (!binding.edits.empty()) {
            const std::string value = property->mixed ? std::string{} : property->value;
            binding.edits.front()->setText(std::wstring(value.begin(), value.end()));
            binding.edits.front()->setPlaceholder(property->mixed ? L"<mixed>" : L"");
        }
    }
}

void EditorShell::applyInspectorValue(const std::string& property, const std::string& value) {
    std::string error;
    bool changed = false;
    for (const auto& nodeId : m_session->model().selection().nodeIds) {
        if (!m_session->setProperty(nodeId, property, value, false, error)) {
            break;
        }
        changed = true;
    }
    if (!changed) {
        if (!error.empty())
            setStatus(error);
        return;
    }
    const bool transformOnly =
        property == "position" || property == "size" || property == "scale" || property == "rotation" || property == "visible" || property == "display_layer";
    syncSelectedRuntimeNodes(transformOnly);
    refreshSelectionOverlay();
    refreshInspector();
    setStatus("Applied " + property + " = " + value);
}

void EditorShell::beginPropertyEdit(const std::string& property, const std::string& value) {
    m_editProperty = property;
    m_editValue = value;
    setStatus("Editing " + property + ": type a value and press Enter");
}

void EditorShell::commitPropertyEdit() {
    if (m_editProperty.empty())
        return;
    std::string error;
    bool changed = false;
    for (const auto& nodeId : m_session->model().selection().nodeIds) {
        if (!m_session->setProperty(nodeId, m_editProperty, m_editValue, false, error))
            break;
        changed = true;
    }
    if (changed) {
        const bool transformOnly = m_editProperty == "position" || m_editProperty == "size" || m_editProperty == "scale" || m_editProperty == "rotation" ||
                                   m_editProperty == "visible" || m_editProperty == "display_layer";
        syncSelectedRuntimeNodes(transformOnly);
        refreshSelectionOverlay();
        refreshInspector();
        setStatus("Applied " + m_editProperty + " = " + m_editValue);
    } else if (!error.empty()) {
        setStatus(error);
    }
    m_editProperty.clear();
    m_editValue.clear();
}

void EditorShell::handleChar(unsigned int codepoint) {
    if (m_sceneRenameEdit && codepoint >= 32 && codepoint <= 0x10FFFF) {
        TouchEvent event;
        event.eventType = TOUCH_EVENT_TYPE_CHARACTER;
        event.deviceType = TOUCH_DEVICE_TYPE_KEYBOARD;
        event.unicodeCodepoint = codepoint;
        m_sceneRenameEdit->dispatchTouchEvent(event);
        return;
    }
    if (m_editProperty.empty() || codepoint < 32 || codepoint > 126)
        return;
    m_editValue.push_back(static_cast<char>(codepoint));
    setStatus("Editing " + m_editProperty + ": " + m_editValue);
}

void EditorShell::rebuildRuntime() {
    if (!m_previewRoot)
        return;
    for (const auto& border : m_selectionBorders)
        m_previewRoot->removeChild(border);
    for (const auto& handle : m_selectionHandles)
        m_previewRoot->removeChild(handle);
    m_previewRoot->m_children.clear();
    m_runtimeNodes.clear();
    refreshViewportGuides();
    std::string error;
    const bool runtimeInstantiated = SceneInstantiator::instantiate(m_session->document(), m_previewRoot, &m_assets, error, &m_runtimeNodes);
    if (!runtimeInstantiated) {
        setStatus(error);
    }
    refreshSelectionOverlay();
    if (runtimeInstantiated) {
        m_events.onRuntimeRebuilt.notify();
    }
}

void EditorShell::refreshSelectionOverlay() {
    if (!m_previewRoot)
        return;

    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    if (!selectedRuntimeRect(x, y, width, height)) {
        for (const auto& border : m_selectionBorders)
            border->setVisible(false);
        for (const auto& handle : m_selectionHandles)
            handle->setVisible(false);
        return;
    }

    const Vector4 borderColor(1.0f, 0.34f, 0.18f, 1.0f);
    while (m_selectionBorders.size() < 4) {
        auto border = MRColor::create();
        border->setColor(borderColor);
        border->setDisplayLayer(10);
        m_selectionBorders.push_back(border);
    }
    while (m_selectionHandles.size() < 8) {
        auto handle = MRColor::create();
        handle->setColor(Vector4(1.0f, 0.23f, 0.16f, 1.0f));
        handle->setDisplayLayer(10);
        m_selectionHandles.push_back(handle);
    }
    for (const auto& border : m_selectionBorders) {
        if (border->m_parent != m_previewRoot)
            m_previewRoot->addChild(border);
        border->setVisible(true);
    }
    for (const auto& handle : m_selectionHandles) {
        if (handle->m_parent != m_previewRoot)
            m_previewRoot->addChild(handle);
        handle->setVisible(true);
    }

    const float thickness = 2.0f / std::max(0.2f, m_viewZoom);
    const std::vector<Math::Rect> borders = {
        {x, y, x + width, y + thickness},
        {x, y + height - thickness, x + width, y + height},
        {x, y, x + thickness, y + height},
        {x + width - thickness, y, x + width, y + height},
    };
    for (size_t index = 0; index < borders.size(); ++index) {
        m_selectionBorders[index]->getTransform()->setPosition(borders[index].Min.x, borders[index].Min.y, 0.0f);
        m_selectionBorders[index]->getTransform()->setSize(borders[index].GetWidth(), borders[index].GetHeight());
    }

    const float handleSize = 10.0f / std::max(0.2f, m_viewZoom);
    const std::vector<Vector2> points = {
        {x, y},          {x + width * 0.5f, y},  {x + width, y}, {x + width, y + height * 0.5f}, {x + width, y + height}, {x + width * 0.5f, y + height},
        {x, y + height}, {x, y + height * 0.5f},
    };
    for (size_t index = 0; index < points.size(); ++index) {
        auto handle = std::dynamic_pointer_cast<MRColor>(m_selectionHandles[index]);
        if (handle)
            handle->setRounding(handleSize * 0.5f);
        m_selectionHandles[index]->getTransform()->setPosition(points[index].x - handleSize * 0.5f, points[index].y - handleSize * 0.5f, 0.0f);
        m_selectionHandles[index]->getTransform()->setSize(handleSize, handleSize);
    }
}

bool EditorShell::runtimeNodeRect(const std::string& nodeId, float& x, float& y, float& width, float& height) const {
    if (!m_previewRoot || nodeId.empty())
        return false;

    const auto runtimeNode = m_runtimeNodes.find(nodeId);
    if (runtimeNode == m_runtimeNodes.end())
        return false;

    const auto widget = std::dynamic_pointer_cast<UIWidget>(runtimeNode->second);
    const auto nodeTransform = widget ? widget->getTransform() : nullptr;
    const auto previewTransform = m_previewRoot->getTransform();
    if (!nodeTransform || !previewTransform)
        return false;

    Matrix4 relativeMatrix;
    try {
        relativeMatrix.multiplyMatrices(previewTransform->getWorldMatrix().inverted(), nodeTransform->getWorldMatrix());
    } catch (const std::invalid_argument&) {
        return false;
    }

    const Vector3 nodeSize = nodeTransform->getSize();
    const Vector3 previewSize = previewTransform->getSize();
    const float halfWidth = nodeSize.x * 0.5f;
    const float halfHeight = nodeSize.y * 0.5f;
    std::array<Vector3, 4> corners = {
        Vector3(-halfWidth, -halfHeight, 0.0f),
        Vector3(-halfWidth, halfHeight, 0.0f),
        Vector3(halfWidth, -halfHeight, 0.0f),
        Vector3(halfWidth, halfHeight, 0.0f),
    };

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    for (auto& corner : corners) {
        corner.apply(relativeMatrix);
        const float localX = corner.x + previewSize.x * 0.5f;
        const float localY = previewSize.y * 0.5f - corner.y;
        minX = std::min(minX, localX);
        minY = std::min(minY, localY);
        maxX = std::max(maxX, localX);
        maxY = std::max(maxY, localY);
    }

    x = minX;
    y = minY;
    width = maxX - minX;
    height = maxY - minY;
    return width >= 0.0f && height >= 0.0f;
}

bool EditorShell::selectedRuntimeRect(float& x, float& y, float& width, float& height) const {
    return runtimeNodeRect(m_selectedNodeId, x, y, width, height);
}

std::string EditorShell::runtimeNodeAt(float x, float y) const {
    const auto& nodes = m_session->document().nodes();
    for (auto node = nodes.rbegin(); node != nodes.rend(); ++node) {
        const auto runtimeNode = m_runtimeNodes.find(node->id);
        if (runtimeNode == m_runtimeNodes.end() || !runtimeNode->second || !runtimeNode->second->getVisible())
            continue;

        float nodeX = 0.0f;
        float nodeY = 0.0f;
        float nodeWidth = 0.0f;
        float nodeHeight = 0.0f;
        if (runtimeNodeRect(node->id, nodeX, nodeY, nodeWidth, nodeHeight) && x >= nodeX && y >= nodeY && x <= nodeX + nodeWidth && y <= nodeY + nodeHeight)
            return node->id;
    }
    return {};
}

bool EditorShell::syncRuntimeNode(const std::string& nodeId, bool transformOnly) {
    const auto* record = m_session->document().findNode(nodeId);
    const auto instance = m_runtimeNodes.find(nodeId);
    if (!record || instance == m_runtimeNodes.end())
        return false;
    std::string error;
    const bool updated = transformOnly ? SceneInstantiator::updateNodeTransform(*record, instance->second, error)
                                       : SceneInstantiator::updateNode(m_session->document(), *record, instance->second, &m_assets, error);
    if (!updated && !error.empty())
        setStatus(error);
    return updated;
}

void EditorShell::syncSelectedRuntimeNodes(bool transformOnly) {
    for (const auto& nodeId : m_session->model().selection().nodeIds) {
        syncRuntimeNode(nodeId, transformOnly);
    }
}

ResizeHandle EditorShell::resizeHandleAt(float x, float y, float nodeX, float nodeY, float width, float height) const {
    const float radius = 9.0f / std::max(0.2f, m_viewZoom);
    const std::vector<std::pair<ResizeHandle, Vector2>> handles = {
        {ResizeHandle::TopLeft, {nodeX, nodeY}},
        {ResizeHandle::Top, {nodeX + width * 0.5f, nodeY}},
        {ResizeHandle::TopRight, {nodeX + width, nodeY}},
        {ResizeHandle::Right, {nodeX + width, nodeY + height * 0.5f}},
        {ResizeHandle::BottomRight, {nodeX + width, nodeY + height}},
        {ResizeHandle::Bottom, {nodeX + width * 0.5f, nodeY + height}},
        {ResizeHandle::BottomLeft, {nodeX, nodeY + height}},
        {ResizeHandle::Left, {nodeX, nodeY + height * 0.5f}},
    };
    for (const auto& [handle, point] : handles) {
        const float dx = x - point.x;
        const float dy = y - point.y;
        if (dx * dx + dy * dy <= radius * radius)
            return handle;
    }
    return ResizeHandle::None;
}

void EditorShell::refreshViewportGuides() {
    if (!m_previewRoot)
        return;

    if (m_previewCanvas) {
        m_previewRoot->removeChild(m_previewCanvas);
        m_previewCanvas.reset();
    }
    if (m_previewGrid) {
        m_previewRoot->removeChild(m_previewGrid);
        m_previewGrid.reset();
    }
}

void EditorShell::handleViewportPointer(const TouchEvent& event) {
    const Math::Rect viewportBounds = m_viewportPanel->getScreenSpaceAABB();
    m_viewportX = viewportBounds.Min.x;
    m_viewportY = viewportBounds.Min.y;
    m_viewportWidth = viewportBounds.GetWidth();
    m_viewportHeight = viewportBounds.GetHeight();
    const float panelX = event.positionX - m_viewportX;
    const float panelY = event.positionY - m_viewportY;
    const float x = (panelX - m_viewPanX) / m_viewZoom;
    const float y = (panelY - 32.0f - m_viewPanY) / m_viewZoom;
    if (panelX < 0.0f || panelY < 32.0f || panelX > m_viewportWidth || panelY > m_viewportHeight)
        return;
    if (event.eventType == TOUCH_EVENT_TYPE_WHEEL) {
        const float oldZoom = m_viewZoom;
        m_viewZoom = std::clamp(m_viewZoom * (event.wheelDeltaY > 0.0f ? 1.1f : 0.9f), 0.2f, 5.0f);
        m_viewPanX = panelX - x * m_viewZoom;
        m_viewPanY = panelY - 32.0f - y * m_viewZoom;
        m_previewRoot->getTransform()->setPosition(m_viewPanX, 32.0f + m_viewPanY, 0.0f);
        m_previewRoot->getTransform()->setScale(m_viewZoom, m_viewZoom, 1.0f);
        refreshSelectionOverlay();
        setStatus("Viewport zoom " + std::to_string(m_viewZoom) + " (was " + std::to_string(oldZoom) + ")");
        return;
    }
    if (event.eventType == TOUCH_EVENT_TYPE_TOUCH && event.button == TOUCH_MOUSE_BUTTON_MIDDLE) {
        m_panning = true;
        m_lastPointerX = panelX;
        m_lastPointerY = panelY;
        return;
    }
    if (event.eventType == TOUCH_EVENT_TYPE_MOVE && m_panning) {
        m_viewPanX += panelX - m_lastPointerX;
        m_viewPanY += panelY - m_lastPointerY;
        m_lastPointerX = panelX;
        m_lastPointerY = panelY;
        m_previewRoot->getTransform()->setPosition(m_viewPanX, 32.0f + m_viewPanY, 0.0f);
        return;
    }
    if (event.eventType == TOUCH_EVENT_TYPE_TOUCH && event.button == TOUCH_MOUSE_BUTTON_LEFT) {
        float selectedX = 0.0f;
        float selectedY = 0.0f;
        float selectedWidth = 0.0f;
        float selectedHeight = 0.0f;
        if (selectedRuntimeRect(selectedX, selectedY, selectedWidth, selectedHeight)) {
            const ResizeHandle handle = resizeHandleAt(x, y, selectedX, selectedY, selectedWidth, selectedHeight);
            if (handle != ResizeHandle::None) {
                if (m_session->model().selectedLocalRect(m_resizeNodeStartX, m_resizeNodeStartY, m_resizeNodeStartZ, m_resizeNodeStartWidth, m_resizeNodeStartHeight)) {
                    m_resizeHandle = handle;
                    m_resizing = true;
                    m_dragging = false;
                    m_viewportTransformChanged = false;
                    m_resizePointerStartX = x;
                    m_resizePointerStartY = y;
                    return;
                }
            }
        }
        const bool additive = (event.modifiers & TOUCH_MODIFIER_CTRL) != 0;
        const std::string targetNodeId = runtimeNodeAt(x, y);
        std::string error;
        if (!targetNodeId.empty() && m_session->selectNode(targetNodeId, additive, error)) {
            notifySelectionChanged();
            if (m_leftTabs)
                m_leftTabs->selectTab("scene_tree");
            m_resizing = false;
            m_resizeHandle = ResizeHandle::None;
            m_dragging = true;
            m_viewportTransformChanged = false;
            m_lastPointerX = panelX;
            m_lastPointerY = panelY;
            setStatus("Selected " + targetNodeId);
        }
    } else if (event.eventType == TOUCH_EVENT_TYPE_MOVE && m_resizing && !m_selectedNodeId.empty()) {
        const float dx = x - m_resizePointerStartX;
        const float dy = y - m_resizePointerStartY;
        float nodeX = m_resizeNodeStartX;
        float nodeY = m_resizeNodeStartY;
        float nodeWidth = m_resizeNodeStartWidth;
        float nodeHeight = m_resizeNodeStartHeight;
        const bool left = m_resizeHandle == ResizeHandle::Left || m_resizeHandle == ResizeHandle::TopLeft || m_resizeHandle == ResizeHandle::BottomLeft;
        const bool right = m_resizeHandle == ResizeHandle::Right || m_resizeHandle == ResizeHandle::TopRight || m_resizeHandle == ResizeHandle::BottomRight;
        const bool top = m_resizeHandle == ResizeHandle::Top || m_resizeHandle == ResizeHandle::TopLeft || m_resizeHandle == ResizeHandle::TopRight;
        const bool bottom = m_resizeHandle == ResizeHandle::Bottom || m_resizeHandle == ResizeHandle::BottomLeft || m_resizeHandle == ResizeHandle::BottomRight;
        if (left) {
            nodeX += dx;
            nodeWidth -= dx;
        } else if (right) {
            nodeWidth += dx;
        }
        if (top) {
            nodeY += dy;
            nodeHeight -= dy;
        } else if (bottom) {
            nodeHeight += dy;
        }
        if (nodeWidth < 1.0f) {
            if (left)
                nodeX -= 1.0f - nodeWidth;
            nodeWidth = 1.0f;
        }
        if (nodeHeight < 1.0f) {
            if (top)
                nodeY -= 1.0f - nodeHeight;
            nodeHeight = 1.0f;
        }
        std::string error;
        if (m_session->setNodeRect(m_selectedNodeId, nodeX, nodeY, m_resizeNodeStartZ, nodeWidth, nodeHeight, true, error)) {
            syncRuntimeNode(m_selectedNodeId, true);
            refreshSelectionOverlay();
            m_viewportTransformChanged = true;
        }
    } else if (event.eventType == TOUCH_EVENT_TYPE_MOVE && m_dragging && !m_selectedNodeId.empty()) {
        float dx = (panelX - m_lastPointerX) / m_viewZoom;
        float dy = (panelY - m_lastPointerY) / m_viewZoom;
        std::string error;
        if (m_session->moveSelection(dx, dy, true, error)) {
            syncSelectedRuntimeNodes(true);
            refreshSelectionOverlay();
            m_viewportTransformChanged = true;
            m_lastPointerX = panelX;
            m_lastPointerY = panelY;
        }
    } else if (event.eventType == TOUCH_EVENT_TYPE_RELEASE) {
        const bool transformChanged = m_viewportTransformChanged;
        m_dragging = false;
        m_resizing = false;
        m_resizeHandle = ResizeHandle::None;
        m_panning = false;
        m_viewportTransformChanged = false;
        if (transformChanged)
            refreshInspector();
    }
}

void EditorShell::handleInput(std::vector<TouchEvent>& events) {
    pollBuild();
    for (const auto& event : events) {
        const bool pointerEvent = event.eventType == TOUCH_EVENT_TYPE_TOUCH || event.eventType == TOUCH_EVENT_TYPE_MOVE || event.eventType == TOUCH_EVENT_TYPE_RELEASE ||
                                  event.eventType == TOUCH_EVENT_TYPE_WHEEL;
        if (pointerEvent && m_createNodeDialog && m_createNodeDialog->isOpen()) {
            m_dragging = false;
            m_resizing = false;
            m_resizeHandle = ResizeHandle::None;
            m_panning = false;
            m_viewportTransformChanged = false;
            continue;
        }
        if (event.eventType == TOUCH_EVENT_TYPE_TOUCH) {
            if (m_sceneRenameEdit && !isDescendantOf(event.target, m_sceneRenameEdit)) {
                commitSceneNodeRename();
                continue;
            }
            if (m_sceneContextMenu && m_sceneContextMenu->isOpen() && !isDescendantOf(event.target, m_sceneContextMenu)) {
                m_sceneContextMenu->hide();
            }
            if (m_textureAssetMenu && m_textureAssetMenu->isOpen() && !isDescendantOf(event.target, m_textureAssetMenu)) {
                m_textureAssetMenu->hide();
            }
        }
        if (event.eventType == TOUCH_EVENT_TYPE_KEY_DOWN && event.keyCode == TOUCH_KEY_DELETE && !event.target && (!m_createNodeDialog || !m_createNodeDialog->isOpen()) &&
            !m_sceneRenameEdit) {
            deleteSelectedSceneNode();
            continue;
        }
        if (handleSceneTreeDrag(event))
            continue;
        if (handleDockDrag(event))
            continue;
        const bool activeViewportGesture = m_dragging || m_resizing || m_panning;
        const bool insideViewport = m_viewportPanel && m_viewportPanel->getScreenSpaceAABB().Contains(event.positionX, event.positionY);
        if (pointerEvent && (activeViewportGesture || insideViewport)) {
            handleViewportPointer(event);
        }
    }
}

void EditorShell::handleKey(int key, int action, int mods) {
    if (m_createNodeDialog && m_createNodeDialog->isOpen() && key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        m_createNodeDialog->hideDialog();
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
    if (m_sceneRenameEdit) {
        if (key == GLFW_KEY_ESCAPE) {
            cancelSceneNodeRename();
            return;
        }
        if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
            commitSceneNodeRename();
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
            m_sceneRenameEdit->dispatchTouchEvent(event);
        }
        return;
    }
    if (!m_editProperty.empty()) {
        if (key == GLFW_KEY_ENTER) {
            commitPropertyEdit();
            return;
        }
        if (key == GLFW_KEY_ESCAPE) {
            m_editProperty.clear();
            m_editValue.clear();
            setStatus("Property edit cancelled");
            return;
        }
        if (key == GLFW_KEY_BACKSPACE && !m_editValue.empty()) {
            m_editValue.pop_back();
            setStatus("Editing " + m_editProperty + ": " + m_editValue);
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
        rebuildRuntime();
    setStatus(error.empty() ? "Shortcut applied" : error);
    refreshSceneTree();
    refreshInspector();
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
    runImportQueue();
    refreshSceneTree();
    rebuildRuntime();
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
