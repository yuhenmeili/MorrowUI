#ifndef MORROW_EDITOR_SHELL_H
#define MORROW_EDITOR_SHELL_H

#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include "EditorEvents.h"
#include "ProjectSettings.h"
#include "assets/AssetDatabase.h"
#include "assets/ImportQueue.h"
#include "base/UIWidget.h"
#include "build/BuildQueue.h"
#include "core/Observable.h"
#include "filesystem/ProjectFileSystemModel.h"
#include "layout/DragDropManager.h"
#include "layout/MRSplitContainer.h"
#include "layout/MRTabContainer.h"
#include "scene/EditorSession.h"
#include "scene/NodeTypeCatalog.h"
#include "ui/CreateNodeDialog.h"
#include "ui/DockDropOverlay.h"
#include "ui/DockLayout.h"
#include "ui/FileSystemPanel.h"
#include "wgl/OpenglHeader.h"

namespace morrow {
class Engine;
class Window;
class BaseButton;
class MRButton;
class MRLabel;
class MRLineEdit;
class MRTextEdit;
class MRPopupMenu;
class TouchEvent;
} // namespace morrow

namespace morrow::editor {
enum class PreviewState { Stopped, Starting, Running, Outdated, Failed };

class InspectorPanel;
class SceneTreePanel;
class ViewportPanel;
class ToolbarPanel;
class AssetBrowserPanel;
class OutputPanel;
class BuildPanel;
class EditorLayoutController;

class EditorShell {
public:
    EditorShell(const std::shared_ptr<Window>& window, const std::shared_ptr<Engine>& engine, std::filesystem::path projectPath, std::filesystem::path scenePath,
                std::filesystem::path assetRoot);

    ~EditorShell();

    bool initialize(std::string& error);

    EditorEvents& events();

private:
    friend class InspectorPanel;
    friend class SceneTreePanel;
    friend class ViewportPanel;
    friend class ToolbarPanel;
    friend class AssetBrowserPanel;
    friend class OutputPanel;
    friend class BuildPanel;
    friend class EditorLayoutController;

    void buildLayout();

    bool handleAssetDrag(const TouchEvent& event);

    void handleInput(std::vector<TouchEvent>& events);

    void handleKey(int key, int action, int mods);

    bool handleDockDrag(const TouchEvent& event);

    void applyDockLayout();

    void saveDockLayout();

    void syncDockTabs();

    DockDropZone dropZoneAt(float x, float y) const;

    std::shared_ptr<MRTabContainer> tabContainerForId(const std::string& id) const;

    std::string tabGroupForWidget(const std::shared_ptr<Widget>& widget) const;

    void completeDockDrop(float x, float y);

    void handleFramebufferResize(const Vector2& size);

    void handleChar(unsigned int codepoint);

    void setStatus(const std::string& text);

    void notifySelectionChanged();

    void addLabel(const std::shared_ptr<UIWidget>& parent, const std::string& text, float x, float y, float width, float height);

    std::shared_ptr<MRButton> addButton(const std::shared_ptr<UIWidget>& parent, const std::wstring& text, float x, float y, float width, float height,
                                        std::function<void()> callback);

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    static void charCallback(GLFWwindow* window, unsigned int codepoint);

    std::shared_ptr<Window> m_window;
    std::shared_ptr<Engine> m_engine;
    EditorEvents m_events;
    Observable<const SelectionState&>::Connection m_selectionChangedConnection;
    std::filesystem::path m_projectPath;
    std::filesystem::path m_scenePath;
    std::filesystem::path m_assetRoot;
    std::shared_ptr<UIWidget> m_shellRoot;
    std::unique_ptr<InspectorPanel> m_inspector;
    std::unique_ptr<SceneTreePanel> m_sceneTree;
    std::unique_ptr<ViewportPanel> m_viewport;
    std::unique_ptr<ToolbarPanel> m_toolbar;
    std::unique_ptr<AssetBrowserPanel> m_assetsPanel;
    std::unique_ptr<OutputPanel> m_output;
    std::unique_ptr<BuildPanel> m_build;
    std::unique_ptr<EditorLayoutController> m_layout;
    std::shared_ptr<MRSplitContainer> m_workspaceSplit;
    std::shared_ptr<MRSplitContainer> m_mainSplit;
    std::shared_ptr<MRSplitContainer> m_centerSplit;
    std::shared_ptr<MRTabContainer> m_leftTabs;
    std::shared_ptr<MRTabContainer> m_centerTabs;
    std::shared_ptr<MRTabContainer> m_bottomTabs;
    std::shared_ptr<DockDropOverlay> m_dockDropOverlay;
    std::shared_ptr<EditorSession> m_session;
    AssetDatabase m_assets;
    ProjectFileSystemModel m_fileSystem;
    NodeTypeCatalog m_nodeTypeCatalog;
    ImportQueue m_importQueue;
    BuildQueue m_buildQueue;
    ProjectSettings m_project;
    DockLayout m_dockLayout;
    std::filesystem::path m_dockLayoutPath;
    std::vector<std::string> m_outputLines;
    std::future<BuildTaskResult> m_buildFuture;
    BuildTaskKind m_pendingBuildKind = BuildTaskKind::Build;
    std::filesystem::path m_lastSuccessfulExecutable;
    PreviewState m_previewState = PreviewState::Stopped;
    std::string m_stdoutRemainder;
    std::string m_stderrRemainder;
    Observable<std::vector<TouchEvent>&>::Connection m_inputConnection;
    Observable<const Vector2&>::Connection m_framebufferSizeConnection;
    std::vector<Observable<MRSplitContainer&, float>::Connection> m_splitConnections;
    std::vector<Observable<MRTabContainer&, const std::string&>::Connection> m_tabConnections;
    std::vector<Observable<MRTabContainer&>::Connection> m_tabOrderConnections;
    std::vector<Observable<BaseButton&>::Connection> m_buttonConnections;
    std::vector<Observable<MRTextEdit&, const std::wstring&>::Connection> m_copyConnections;
    std::string m_status;
    std::string m_selectedNodeId;
    std::string m_pendingAssetId;
    std::shared_ptr<Widget> m_pendingAssetSource;
    std::shared_ptr<MRButton> m_assetDragPreview;
    float m_assetDragStartX = 0.0f;
    float m_assetDragStartY = 0.0f;
    std::vector<std::string> m_lastNotifiedSelection;
    DragDropManager m_dragDrop;
    std::string m_pendingDockTab;
    std::string m_pendingDockGroup;
    float m_pendingDockX = 0.0f;
    float m_pendingDockY = 0.0f;
};
} // namespace morrow::editor

#endif  // MORROW_EDITOR_SHELL_H
