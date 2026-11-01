#ifndef MORROW_EDITOR_VIEWPORT_PANEL_H
#define MORROW_EDITOR_VIEWPORT_PANEL_H

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "panels/EditorPanel.h"

namespace morrow {
class MRButton;
class MRColor;
class MRLabel;
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

/// 标尺条厚度与视口顶栏留白（panel 局部系 px）。顶栏占两行（标题/对齐 + 预设下拉），
/// 其下是标尺条，画布内容从 kViewportTopInset 开始。
constexpr float kRulerThickness = 16.0f;
constexpr float kViewportTopInset = 68.0f;

class ViewportPanel : public EditorPanel {
public:
    explicit ViewportPanel(EditorShell& shell);

    void rebuildRuntime();

    void refreshSelectionOverlay();

    void syncSelectedRuntimeNodes(bool transformOnly);

    bool syncRuntimeNode(const std::string& nodeId, bool transformOnly);

    /// 重建网格/参考线/标尺叠加层（pan/zoom/文档变化后调用）。
    void refreshGuides();

    void resizeToPanel();

    /// 平移/缩放视口使选中节点并集矩形居中适配（F 键）。
    void focusSelection();

    /// 画布预设：0 = Fit（随面板自适应），>0 固定分辨率并按内容缩放居中，
    /// 触发容器/锚点真实重排（多分辨率预览）。
    void applyCanvasPreset(int presetIndex);

    int currentCanvasPreset() const;

    void handlePointer(const TouchEvent& event);

private:
    friend class EditorShell;
    friend class ToolbarPanel;
    friend class CommandPalette;

    std::shared_ptr<UIWidget>& panel = m_root;
    std::shared_ptr<UIWidget> previewRoot;
    std::shared_ptr<UIWidget> previewCanvas;
    std::shared_ptr<UIWidget> previewGrid;
    std::vector<std::shared_ptr<UIWidget>> selectionBorders;
    std::vector<std::shared_ptr<UIWidget>> selectionHandles;
    std::unordered_map<std::string, std::shared_ptr<Widget>> runtimeNodes;
    bool dragging = false;
    bool resizing = false;
    bool rotating = false;
    bool panning = false;
    bool rubberBanding = false;
    bool rubberAdditive = false;
    bool transformChanged = false;
    // ---- 锚定节点的 gizmo 行为 ----
    bool dragAnchored = false;
    bool resizeAnchored = false;
    // ---- 吸附（网格 + 智能对齐） ----
    bool snapEnabled = true;
    float snapStep = 10.0f;
    float dragStartPointerX = 0.0f;
    float dragStartPointerY = 0.0f;
    float dragPrimaryStartX = 0.0f;
    float dragPrimaryStartY = 0.0f;
    float dragPrimaryStartWidth = 0.0f;
    float dragPrimaryStartHeight = 0.0f;
    float dragAppliedDx = 0.0f;
    float dragAppliedDy = 0.0f;
    // ---- 参考线 / 标尺 / 网格 / 对齐线叠加 ----
    std::vector<std::shared_ptr<morrow::MRColor>> guideLines;
    std::vector<std::shared_ptr<morrow::MRColor>> gridLines;
    std::vector<std::shared_ptr<morrow::MRColor>> smartLines;
    std::vector<std::shared_ptr<morrow::MRColor>> rulerBackgrounds;
    std::vector<std::shared_ptr<morrow::MRLabel>> rulerLabels;
    /// 多选对齐/分布按钮（挂在 panel 顶栏，选中 ≥2 时可见）。
    std::vector<std::shared_ptr<morrow::MRButton>> alignButtons;
    /// 吸附开关/步长按钮（EditorShell 创建于顶栏第二行，文案由 shell 维护）。
    std::shared_ptr<morrow::MRButton> snapButton;
    std::shared_ptr<morrow::MRButton> stepButton;
    /// 锚点指示方块（选中锚定节点时显示在父矩形上，最多 4 个角）。
    std::vector<std::shared_ptr<morrow::MRColor>> anchorIndicators;
    // ---- 分辨率/安全区预览 ----
    int canvasPreset = 0;
    std::vector<std::shared_ptr<morrow::MRColor>> canvasBorders;
    std::vector<std::shared_ptr<morrow::MRColor>> safeAreaBars;
    bool draggingGuide = false;
    bool draggingGuideHorizontal = false;
    bool draggingExistingGuide = false;
    std::shared_ptr<morrow::MRColor> guideDragPreview;
    ResizeHandle resizeHandle = ResizeHandle::None;
    float rotateStartPointerAngle = 0.0f;
    float rotateStartRotationZ = 0.0f;
    float rotatePivotX = 0.0f;
    float rotatePivotY = 0.0f;
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
    std::vector<std::string> rubberBaseSelection;
    float rubberStartX = 0.0f;
    float rubberStartY = 0.0f;
    float rubberCurrentX = 0.0f;
    float rubberCurrentY = 0.0f;
    std::shared_ptr<morrow::MRColor> rubberBand;

    bool runtimeNodeRect(const std::string& nodeId, float& x, float& y, float& width, float& height) const;

    bool selectedRuntimeRect(float& x, float& y, float& width, float& height) const;

    std::string runtimeNodeAt(float x, float y) const;

    ResizeHandle resizeHandleAt(float x, float y, float nodeX, float nodeY, float width, float height) const;

    bool rotationHandleAt(float x, float y, float nodeX, float nodeY, float width, float height) const;

    /// 更新/隐藏橡皮筋矩形（previewRoot 局部系）。
    void updateRubberBandOverlay();

    /// 框选收尾：与所有节点世界矩形求交并更新选择集。
    void finishRubberBandSelection();

    /// 网格吸附一个画布坐标值。
    float snapValue(float value) const;

    /// 拖拽位移网格吸附：基于主选中节点起始位置取整。
    void applyGridSnapToDelta(float& dx, float& dy) const;

    /// 智能对齐：主选中节点三边与其它节点边缘/中线/参考线匹配则吸附，并记录命中的线。
    void applySmartSnapToDelta(float& dx, float& dy, std::vector<std::pair<char, float>>& matchedLines) const;

    /// 显示/隐藏智能对齐辅助线。
    void showSmartLines(const std::vector<std::pair<char, float>>& lines);

    void hideSmartLines();

    /// 参考线拖拽预览（从标尺拖出，跟随光标）。
    void updateGuideDragPreview(float value);

    /// 标尺条命中检测（panel 局部系）。
    bool inTopRuler(float panelX, float panelY) const;

    bool inLeftRuler(float panelX, float panelY) const;

    /// 现有参考线命中（画布系）；命中时 horizontal/value 给出该线。
    bool guideAt(float canvasX, float canvasY, bool& horizontal, float& value) const;

    /// 文档中节点是否带完整锚点键（anchor_min + anchor_max）。
    bool nodeAnchored(const std::string& nodeId) const;

    /// 锚定节点的目标矩形 → 换算 anchor_offset 写回文档（保持锚点不变）。
    bool applyAnchoredRect(const std::string& nodeId, float x, float y, float width, float height, bool continuous);

    /// 读取节点锚点（画布/文档空间换算用），无锚点返回 false。
    bool nodeAnchorValues(const std::string& nodeId, float& minX, float& minY, float& maxX, float& maxY, float& parentWidth, float& parentHeight) const;

    /// 重建标尺刻度（panel 子控件，随 pan/zoom 更新）。
    void rebuildRulers();

    void clearRulers();

    EditorShell& m_shell;
};
}  // namespace morrow::editor

#endif  // MORROW_EDITOR_VIEWPORT_PANEL_H
