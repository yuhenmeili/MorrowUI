#ifndef MORROW_EDITOR_SHELL_H
#define MORROW_EDITOR_SHELL_H

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "assets/AssetDatabase.h"
#include "assets/ImportQueue.h"
#include "base/UIWidget.h"
#include "scene/EditorSession.h"
#include "wgl/OpenglHeader.h"

namespace morrow {
class Engine;
class Window;
class MRButton;
class MRLabel;
class TouchEvent;
}  // namespace morrow

namespace morrow::editor {

class EditorShell {
public:
    EditorShell(const std::shared_ptr<Window>& window, const std::shared_ptr<Engine>& engine, std::filesystem::path projectPath, std::filesystem::path scenePath,
                std::filesystem::path assetRoot);
    ~EditorShell();

    bool initialize(std::string& error);

private:
    void buildLayout();
    void rebuildRuntime();
    void refreshSceneTree();
    void refreshInspector();
    void handleInput(std::vector<TouchEvent>& events);
    void handleKey(int key, int action, int mods);
    void handleViewportPointer(const TouchEvent& event);
    void setStatus(const std::string& text);
    void runImportQueue();
    void addLabel(const std::shared_ptr<UIWidget>& parent, const std::string& text, float x, float y, float width, float height);
    std::shared_ptr<MRButton> addButton(const std::shared_ptr<UIWidget>& parent, const std::wstring& text, float x, float y, float width, float height,
                                        std::function<void()> callback);

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    std::shared_ptr<Window> m_window;
    std::shared_ptr<Engine> m_engine;
    std::filesystem::path m_projectPath;
    std::filesystem::path m_scenePath;
    std::filesystem::path m_assetRoot;
    std::shared_ptr<UIWidget> m_shellRoot;
    std::shared_ptr<UIWidget> m_previewRoot;
    std::shared_ptr<UIWidget> m_sceneTreePanel;
    std::shared_ptr<UIWidget> m_inspectorPanel;
    std::shared_ptr<UIWidget> m_viewportPanel;
    std::shared_ptr<UIWidget> m_statusPanel;
    std::shared_ptr<EditorSession> m_session;
    AssetDatabase m_assets;
    ImportQueue m_importQueue;
    std::string m_inputObserver;
    std::string m_status;
    std::string m_selectedNodeId;
    bool m_dragging = false;
    float m_lastPointerX = 0.0f;
    float m_lastPointerY = 0.0f;
    float m_viewportX = 240.0f;
    float m_viewportY = 40.0f;
    float m_viewportWidth = 740.0f;
    float m_viewportHeight = 580.0f;
};

}  // namespace morrow::editor

#endif  // MORROW_EDITOR_SHELL_H
