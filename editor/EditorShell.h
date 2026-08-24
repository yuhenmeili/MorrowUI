#ifndef MORROW_EDITOR_SHELL_H
#define MORROW_EDITOR_SHELL_H

#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "assets/AssetDatabase.h"
#include "assets/ImportQueue.h"
#include "base/UIWidget.h"
#include "build/BuildQueue.h"
#include "core/Observable.h"
#include "EditorEvents.h"
#include "filesystem/ProjectFileSystemModel.h"
#include "ProjectSettings.h"
#include "scene/EditorSession.h"
#include "ui/DockLayout.h"
#include "ui/FileSystemPanel.h"
#include "ui/DockDropOverlay.h"
#include "layout/MRSplitContainer.h"
#include "layout/MRTabContainer.h"
#include "layout/DragDropManager.h"
#include "wgl/OpenglHeader.h"

namespace morrow {
class Engine;
class Window;
class BaseButton;
class MRButton;
class MRLabel;
class TouchEvent;
}  // namespace morrow

namespace morrow::editor {

enum class PreviewState { Stopped, Starting, Running, Outdated, Failed };

class EditorShell {
public:
    EditorShell(const std::shared_ptr<Window>& window, const std::shared_ptr<Engine>& engine, std::filesystem::path projectPath, std::filesystem::path scenePath,
                std::filesystem::path assetRoot);
    ~EditorShell();

    bool initialize(std::string& error);

    EditorEvents& events();

private:
    void buildLayout();
    void rebuildRuntime();
    void refreshViewportGuides();
    void refreshSceneTree();
    void refreshInspector();
    void handleInput(std::vector<TouchEvent>& events);
    void handleKey(int key, int action, int mods);
    void handleViewportPointer(const TouchEvent& event);
    bool handleDockDrag(const TouchEvent& event);
    void applyDockLayout();
    void saveDockLayout();
    void syncDockTabs();
    DockDropZone dropZoneAt(float x, float y) const;
    std::shared_ptr<MRTabContainer> tabContainerForId(
        const std::string& id) const;
    std::string tabGroupForWidget(
        const std::shared_ptr<Widget>& widget) const;
    void completeDockDrop(float x, float y);
    void handleFramebufferResize(const Vector2& size);
    void refreshOutput();
    void runBuild(BuildTaskKind kind);
    void pollBuild();
    void stopPreview();
    void setPreviewState(PreviewState state);
    void appendBuildResult(const BuildTaskResult& result);
    void showAssetBrowser();
    void beginPropertyEdit(const std::string& property, const std::string& value);
    void commitPropertyEdit();
    void handleChar(unsigned int codepoint);
    void setStatus(const std::string& text);
    void runImportQueue();
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
    std::shared_ptr<UIWidget> m_toolbarPanel;
    std::shared_ptr<UIWidget> m_previewRoot;
    std::shared_ptr<UIWidget> m_previewCanvas;
    std::shared_ptr<UIWidget> m_previewGrid;
    std::shared_ptr<UIWidget> m_selectionFrame;
    std::shared_ptr<UIWidget> m_sceneTreePanel;
    std::shared_ptr<UIWidget> m_inspectorPanel;
    std::shared_ptr<UIWidget> m_viewportPanel;
    std::shared_ptr<UIWidget> m_statusPanel;
    std::shared_ptr<UIWidget> m_buildPanel;
    std::shared_ptr<FileSystemPanel> m_fileSystemPanel;
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
    std::vector<Observable<MRTabContainer&, const std::string&>::Connection>
        m_tabConnections;
    std::vector<Observable<MRTabContainer&>::Connection>
        m_tabOrderConnections;
    std::vector<Observable<BaseButton&>::Connection> m_buttonConnections;
    std::string m_status;
    std::string m_selectedNodeId;
    std::vector<std::string> m_lastNotifiedSelection;
    bool m_dragging = false;
    bool m_resizing = false;
    bool m_panning = false;
    float m_lastPointerX = 0.0f;
    float m_lastPointerY = 0.0f;
    float m_viewPanX = 0.0f;
    float m_viewPanY = 0.0f;
    float m_viewZoom = 1.0f;
    std::string m_editProperty;
    std::string m_editValue;
    float m_viewportX = 240.0f;
    float m_viewportY = 40.0f;
    float m_viewportWidth = 740.0f;
    float m_viewportHeight = 580.0f;
    DragDropManager m_dragDrop;
    std::string m_pendingDockTab;
    std::string m_pendingDockGroup;
    float m_pendingDockX = 0.0f;
    float m_pendingDockY = 0.0f;
};

}  // namespace morrow::editor

#endif  // MORROW_EDITOR_SHELL_H
