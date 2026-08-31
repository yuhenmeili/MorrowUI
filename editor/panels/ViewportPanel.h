#ifndef MORROW_EDITOR_VIEWPORT_PANEL_H
#define MORROW_EDITOR_VIEWPORT_PANEL_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace morrow {
class TouchEvent;
class UIWidget;
class Widget;
} // namespace morrow

namespace morrow::editor {
class EditorShell;

enum class ResizeHandle {
    None,
    TopLeft,
    Top,
    TopRight,
    Right,
    BottomRight,
    Bottom,
    BottomLeft,
    Left,
};

class ViewportPanel {
public:
    explicit ViewportPanel(EditorShell& shell);

    void rebuildRuntime();

    void refreshSelectionOverlay();

    void syncSelectedRuntimeNodes(bool transformOnly);

    bool syncRuntimeNode(const std::string& nodeId, bool transformOnly);

    void refreshGuides();

    void handlePointer(const TouchEvent& event);

private:
    friend class EditorShell;

    std::shared_ptr<UIWidget> panel;
    std::shared_ptr<UIWidget> previewRoot;
    std::shared_ptr<UIWidget> previewCanvas;
    std::shared_ptr<UIWidget> previewGrid;
    std::vector<std::shared_ptr<UIWidget>> selectionBorders;
    std::vector<std::shared_ptr<UIWidget>> selectionHandles;
    std::unordered_map<std::string, std::shared_ptr<Widget>> runtimeNodes;
    bool dragging = false;
    bool resizing = false;
    bool panning = false;
    bool transformChanged = false;
    ResizeHandle resizeHandle = ResizeHandle::None;
    float resizePointerStartX = 0.0f;
    float resizePointerStartY = 0.0f;
    float resizeNodeStartX = 0.0f;
    float resizeNodeStartY = 0.0f;
    float resizeNodeStartZ = 0.0f;
    float resizeNodeStartWidth = 0.0f;
    float resizeNodeStartHeight = 0.0f;
    float lastPointerX = 0.0f;
    float lastPointerY = 0.0f;
    float panX = 0.0f;
    float panY = 0.0f;
    float zoom = 1.0f;
    float x = 240.0f;
    float y = 40.0f;
    float width = 740.0f;
    float height = 580.0f;

    bool runtimeNodeRect(const std::string& nodeId, float& x, float& y, float& width, float& height) const;

    bool selectedRuntimeRect(float& x, float& y, float& width, float& height) const;

    std::string runtimeNodeAt(float x, float y) const;

    ResizeHandle resizeHandleAt(float x, float y, float nodeX, float nodeY, float width, float height) const;

    EditorShell& m_shell;
};
} // namespace morrow::editor

#endif  // MORROW_EDITOR_VIEWPORT_PANEL_H