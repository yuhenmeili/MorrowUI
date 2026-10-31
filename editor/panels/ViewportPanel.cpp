#include "panels/ViewportPanel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <string>

#include "EditorShell.h"
#include "morrow/base/Interaction.h"
#include "morrow/base/TouchEvent.h"
#include "morrow/base/Transform.h"
#include "morrow/base/UIWidget.h"
#include "morrow/elements/MRColor.h"
#include "morrow/elements/MRLabel.h"
#include "panels/InspectorPanel.h"
#include "scene/SceneInstantiator.h"

namespace morrow::editor {

namespace {
constexpr size_t kRotateHandleIndex = 8;
constexpr float kRotateHandleGap = 26.0f;
constexpr float kGuideHitPixels = 6.0f;
constexpr float kSmartSnapPixels = 6.0f;

/// 画布预设表：index 0 = Fit（面板自适应），其余固定分辨率（宽,高）。
struct CanvasPresetEntry {
    const char* name;
    float width;
    float height;
};
const CanvasPresetEntry kCanvasPresets[] = {
    {"Fit", 0.0f, 0.0f}, {"1920 x 1080", 1920.0f, 1080.0f}, {"1280 x 720", 1280.0f, 720.0f},
    {"800 x 480", 800.0f, 480.0f},   {"480 x 800", 480.0f, 800.0f},     {"1920 x 720", 1920.0f, 720.0f},
};
constexpr int kCanvasPresetCount = sizeof(kCanvasPresets) / sizeof(kCanvasPresets[0]);

std::string rotationZValue(float zDegrees) {
    char buffer[96];
    std::snprintf(buffer, sizeof(buffer), "Vector3(0.000, 0.000, %.3f)", zDegrees);
    return buffer;
}

bool parseVector3Property(const SceneNodeRecord* node, const char* key, float& x, float& y, float& z) {
    x = 0.0f;
    y = 0.0f;
    z = 0.0f;
    if (!node)
        return true;
    const auto entry = node->properties.find(key);
    if (entry == node->properties.end())
        return true;
    const std::string& value = entry->second;
    if (value.rfind("Vector3(", 0) != 0 || value.empty() || value.back() != ')')
        return false;
    const std::string body = value.substr(8, value.size() - 9);
    std::vector<float> components;
    size_t start = 0;
    while (start <= body.size()) {
        const auto separator = body.find(',', start);
        try {
            components.push_back(std::stof(body.substr(start, separator == std::string::npos ? std::string::npos : separator - start)));
        } catch (...) {
            return false;
        }
        if (separator == std::string::npos)
            break;
        start = separator + 1;
    }
    if (components.size() != 3)
        return false;
    x = components[0];
    y = components[1];
    z = components[2];
    return true;
}

bool parseVector2Property(const SceneNodeRecord* node, const char* key, float& x, float& y) {
    x = 0.0f;
    y = 0.0f;
    if (!node)
        return true;
    const auto entry = node->properties.find(key);
    if (entry == node->properties.end())
        return true;
    const std::string& value = entry->second;
    if (value.rfind("Vector2(", 0) != 0 || value.empty() || value.back() != ')')
        return false;
    const std::string body = value.substr(8, value.size() - 9);
    std::vector<float> components;
    size_t start = 0;
    while (start <= body.size()) {
        const auto separator = body.find(',', start);
        try {
            components.push_back(std::stof(body.substr(start, separator == std::string::npos ? std::string::npos : separator - start)));
        } catch (...) {
            return false;
        }
        if (separator == std::string::npos)
            break;
        start = separator + 1;
    }
    if (components.size() != 2)
        return false;
    x = components[0];
    y = components[1];
    return true;
}

std::string tickLabel(float value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%d", static_cast<int>(std::lround(value)));
    return buffer;
}
}  // namespace

ViewportPanel::ViewportPanel(EditorShell& shell) : m_shell(shell) {
}

void ViewportPanel::rebuildRuntime() {
    if (!previewRoot)
        return;
    for (const auto& border : selectionBorders)
        previewRoot->removeChild(border);
    for (const auto& handle : selectionHandles)
        previewRoot->removeChild(handle);
    for (const auto& indicator : anchorIndicators)
        previewRoot->removeChild(indicator);
    previewRoot->m_children.clear();
    guideLines.clear();
    gridLines.clear();
    smartLines.clear();
    if (guideDragPreview && guideDragPreview->m_parent == previewRoot)
        guideDragPreview.reset();
    rubberBanding = false;
    rubberBand.reset();
    runtimeNodes.clear();
    refreshGuides();
    std::string error;
    const bool runtimeInstantiated = SceneInstantiator::instantiate(m_shell.m_session->document(), previewRoot, &m_shell.m_assets, error, &runtimeNodes);
    if (!runtimeInstantiated) {
        m_shell.setStatus(error);
    }
    refreshSelectionOverlay();
    if (runtimeInstantiated) {
        m_shell.m_events.onRuntimeRebuilt.notify();
    }
}

void ViewportPanel::refreshSelectionOverlay() {
    if (!previewRoot)
        return;

    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    if (!selectedRuntimeRect(x, y, width, height)) {
        for (const auto& border : selectionBorders)
            border->setVisible(false);
        for (const auto& handle : selectionHandles)
            handle->setVisible(false);
        for (const auto& indicator : anchorIndicators)
            indicator->setVisible(false);
        return;
    }

    const Vector4 borderColor(1.0f, 0.34f, 0.18f, 1.0f);
    while (selectionBorders.size() < 4) {
        auto border = MRColor::create();
        border->setColor(borderColor);
        border->setDisplayLayer(10);
        selectionBorders.push_back(border);
    }
    while (selectionHandles.size() < 9) {
        auto handle = MRColor::create();
        handle->setColor(Vector4(1.0f, 0.23f, 0.16f, 1.0f));
        handle->setDisplayLayer(10);
        selectionHandles.push_back(handle);
    }
    for (const auto& border : selectionBorders) {
        if (border->m_parent != previewRoot)
            previewRoot->addChild(border);
        border->setVisible(true);
    }
    for (const auto& handle : selectionHandles) {
        if (handle->m_parent != previewRoot)
            previewRoot->addChild(handle);
        handle->setVisible(true);
    }

    const float thickness = 2.0f / std::max(0.2f, zoom);
    const std::vector<Math::Rect> borders = {
        {x, y, x + width, y + thickness},
        {x, y + height - thickness, x + width, y + height},
        {x, y, x + thickness, y + height},
        {x + width - thickness, y, x + width, y + height},
    };
    for (size_t index = 0; index < borders.size(); ++index) {
        selectionBorders[index]->getTransform()->setPosition(borders[index].Min.x, borders[index].Min.y, 0.0f);
        selectionBorders[index]->getTransform()->setSize(borders[index].GetWidth(), borders[index].GetHeight());
    }

    const float handleSize = 10.0f / std::max(0.2f, zoom);
    const float rotateGap = kRotateHandleGap / std::max(0.2f, zoom);
    const std::vector<Vector2> points = {
        {x, y},                            {x + width * 0.5f, y},          {x + width, y},           {x + width, y + height * 0.5f},
        {x + width, y + height},           {x + width * 0.5f, y + height}, {x, y + height},          {x, y + height * 0.5f},
        {x + width * 0.5f, y - rotateGap},
    };
    for (size_t index = 0; index < points.size(); ++index) {
        auto handle = std::dynamic_pointer_cast<MRColor>(selectionHandles[index]);
        if (handle)
            handle->setRounding(handleSize * 0.5f);
        selectionHandles[index]->getTransform()->setPosition(points[index].x - handleSize * 0.5f, points[index].y - handleSize * 0.5f, 0.0f);
        selectionHandles[index]->getTransform()->setSize(handleSize, handleSize);
    }
    (void)kRotateHandleIndex;

    // 对齐/分布按钮可见性：多选时出现
    const size_t selectionCount = m_shell.m_session->model().selection().nodeIds.size();
    for (const auto& button : alignButtons)
        button->setVisible(selectionCount >= 2);

    // 锚点指示：选中锚定节点时，在其父矩形的锚点位置画绿色角标（4 个角）
    float anchorMinX = 0.0f;
    float anchorMinY = 0.0f;
    float anchorMaxX = 0.0f;
    float anchorMaxY = 0.0f;
    float parentWidth = 0.0f;
    float parentHeight = 0.0f;
    const bool anchored = !m_shell.m_selectedNodeId.empty() && nodeAnchorValues(m_shell.m_selectedNodeId, anchorMinX, anchorMinY, anchorMaxX, anchorMaxY, parentWidth, parentHeight);
    if (anchored) {
        // 父矩形 preview-local：优先父 runtime 矩形，根父级用画布 (0,0)
        float baseX = 0.0f;
        float baseY = 0.0f;
        float rectW = parentWidth;
        float rectH = parentHeight;
        const auto* node = m_shell.m_session->document().findNode(m_shell.m_selectedNodeId);
        if (node && !node->parentId.empty()) {
            float ignoredX = 0.0f;
            float ignoredY = 0.0f;
            if (runtimeNodeRect(node->parentId, ignoredX, ignoredY, rectW, rectH)) {
                baseX = ignoredX;
                baseY = ignoredY;
            } else {
                rectW = parentWidth;
                rectH = parentHeight;
            }
        }
        while (anchorIndicators.size() < 4) {
            auto indicator = MRColor::create();
            indicator->setColor(Vector4(0.25f, 0.85f, 0.45f, 1.0f));
            indicator->setDisplayLayer(10);
            anchorIndicators.push_back(indicator);
        }
        for (const auto& indicator : anchorIndicators) {
            if (indicator->m_parent != previewRoot)
                previewRoot->addChild(indicator);
        }
        const float indicatorSize = 8.0f / std::max(0.2f, zoom);
        const float corners[4][2] = {{anchorMinX, anchorMinY}, {anchorMaxX, anchorMinY}, {anchorMaxX, anchorMaxY}, {anchorMinX, anchorMaxY}};
        for (size_t index = 0; index < 4; ++index) {
            const float pointX = baseX + corners[index][0] * rectW;
            const float pointY = baseY + corners[index][1] * rectH;
            anchorIndicators[index]->setVisible(true);
            if (auto indicator = std::dynamic_pointer_cast<MRColor>(anchorIndicators[index]))
                indicator->setRounding(indicatorSize * 0.5f);
            anchorIndicators[index]->getTransform()->setPosition(pointX - indicatorSize * 0.5f, pointY - indicatorSize * 0.5f, 0.0f);
            anchorIndicators[index]->getTransform()->setSize(indicatorSize, indicatorSize);
        }
    } else {
        for (const auto& indicator : anchorIndicators)
            indicator->setVisible(false);
    }
}

bool ViewportPanel::runtimeNodeRect(const std::string& nodeId, float& x, float& y, float& width, float& height) const {
    if (!previewRoot || nodeId.empty())
        return false;

    const auto runtimeNode = runtimeNodes.find(nodeId);
    if (runtimeNode == runtimeNodes.end())
        return false;

    const auto widget = std::dynamic_pointer_cast<UIWidget>(runtimeNode->second);
    const auto nodeTransform = widget ? widget->getTransform() : nullptr;
    const auto previewTransform = previewRoot->getTransform();
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

bool ViewportPanel::selectedRuntimeRect(float& x, float& y, float& width, float& height) const {
    return runtimeNodeRect(m_shell.m_selectedNodeId, x, y, width, height);
}

std::string ViewportPanel::runtimeNodeAt(float x, float y) const {
    const auto& document = m_shell.m_session->document();
    const auto& nodes = document.nodes();
    for (auto node = nodes.rbegin(); node != nodes.rend(); ++node) {
        if (document.isNodeLocked(node->id))
            continue;
        const auto runtimeNode = runtimeNodes.find(node->id);
        if (runtimeNode == runtimeNodes.end() || !runtimeNode->second || !runtimeNode->second->getVisible())
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

bool ViewportPanel::syncRuntimeNode(const std::string& nodeId, bool transformOnly) {
    const auto* record = m_shell.m_session->document().findNode(nodeId);
    const auto instance = runtimeNodes.find(nodeId);
    if (!record || instance == runtimeNodes.end())
        return false;
    std::string error;
    const bool updated = transformOnly ? SceneInstantiator::updateNodeTransform(*record, instance->second, error)
                                       : SceneInstantiator::updateNode(m_shell.m_session->document(), *record, instance->second, &m_shell.m_assets, error);
    if (!updated && !error.empty())
        m_shell.setStatus(error);
    return updated;
}

void ViewportPanel::syncSelectedRuntimeNodes(bool transformOnly) {
    for (const auto& nodeId : m_shell.m_session->model().selection().nodeIds) {
        syncRuntimeNode(nodeId, transformOnly);
    }
}

ResizeHandle ViewportPanel::resizeHandleAt(float x, float y, float nodeX, float nodeY, float width, float height) const {
    const float radius = 9.0f / std::max(0.2f, zoom);
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

bool ViewportPanel::rotationHandleAt(float x, float y, float nodeX, float nodeY, float width, float height) const {
    const float radius = 11.0f / std::max(0.2f, zoom);
    const float gap = kRotateHandleGap / std::max(0.2f, zoom);
    const float dx = x - (nodeX + width * 0.5f);
    const float dy = y - (nodeY - gap);
    return dx * dx + dy * dy <= radius * radius;
}

void ViewportPanel::updateRubberBandOverlay() {
    if (!previewRoot)
        return;
    if (!rubberBanding) {
        if (rubberBand)
            rubberBand->setVisible(false);
        return;
    }
    if (!rubberBand) {
        rubberBand = MRColor::create();
        rubberBand->setColor(Vector4(0.20f, 0.50f, 1.00f, 0.12f));
        rubberBand->setDisplayLayer(11);
        previewRoot->addChild(rubberBand);
    }
    if (rubberBand->m_parent != previewRoot)
        previewRoot->addChild(rubberBand);
    const float left = std::min(rubberStartX, rubberCurrentX);
    const float top = std::min(rubberStartY, rubberCurrentY);
    rubberBand->setVisible(true);
    rubberBand->getTransform()->setPosition(left, top, 0.0f);
    rubberBand->getTransform()->setSize(std::max(1.0f, std::abs(rubberCurrentX - rubberStartX)), std::max(1.0f, std::abs(rubberCurrentY - rubberStartY)));
}

void ViewportPanel::finishRubberBandSelection() {
    const float left = std::min(rubberStartX, rubberCurrentX);
    const float top = std::min(rubberStartY, rubberCurrentY);
    const float right = std::max(rubberStartX, rubberCurrentX);
    const float bottom = std::max(rubberStartY, rubberCurrentY);
    auto& model = m_shell.m_session->model();
    model.clearSelection();
    std::string error;
    // Ctrl 框选保留按下前已有的选择
    if (rubberAdditive) {
        for (const auto& nodeId : rubberBaseSelection)
            model.selectNode(nodeId, true, error);
    }
    for (const auto& node : m_shell.m_session->document().nodes()) {
        float nodeX = 0.0f;
        float nodeY = 0.0f;
        float nodeWidth = 0.0f;
        float nodeHeight = 0.0f;
        if (!runtimeNodeRect(node.id, nodeX, nodeY, nodeWidth, nodeHeight))
            continue;
        const bool intersects = nodeX < right && nodeX + nodeWidth > left && nodeY < bottom && nodeY + nodeHeight > top;
        if (intersects)
            model.selectNode(node.id, true, error);
    }
    m_shell.m_inspector->clearAsset();
    m_shell.notifySelectionChanged();
    const size_t count = model.selection().nodeIds.size();
    m_shell.setStatus(count == 0 ? "Cleared selection" : "Rubber band selected " + std::to_string(count) + " node(s)");
}

float ViewportPanel::snapValue(float value) const {
    if (snapStep <= 0.0f)
        return value;
    return std::round(value / snapStep) * snapStep;
}

void ViewportPanel::applyGridSnapToDelta(float& dx, float& dy) const {
    dx = snapValue(dragPrimaryStartX + dx) - dragPrimaryStartX;
    dy = snapValue(dragPrimaryStartY + dy) - dragPrimaryStartY;
}

void ViewportPanel::applySmartSnapToDelta(float& dx, float& dy, std::vector<std::pair<char, float>>& matchedLines) const {
    const auto& document = m_shell.m_session->document();
    const auto& selection = m_shell.m_session->model().selection().nodeIds;
    const float threshold = kSmartSnapPixels / std::max(0.2f, zoom);

    std::vector<float> candidatesX;
    std::vector<float> candidatesY;
    for (const auto& node : document.nodes()) {
        if (std::find(selection.begin(), selection.end(), node.id) != selection.end())
            continue;
        if (document.isNodeLocked(node.id))
            continue;
        float nodeX = 0.0f;
        float nodeY = 0.0f;
        float nodeWidth = 0.0f;
        float nodeHeight = 0.0f;
        if (!runtimeNodeRect(node.id, nodeX, nodeY, nodeWidth, nodeHeight))
            continue;
        candidatesX.push_back(nodeX);
        candidatesX.push_back(nodeX + nodeWidth * 0.5f);
        candidatesX.push_back(nodeX + nodeWidth);
        candidatesY.push_back(nodeY);
        candidatesY.push_back(nodeY + nodeHeight * 0.5f);
        candidatesY.push_back(nodeY + nodeHeight);
    }
    for (const float guide : document.guideXs())
        candidatesX.push_back(guide);
    for (const float guide : document.guideYs())
        candidatesY.push_back(guide);

    // 主选中节点的三条边（起始位置 + 当前位移），逐轴找最近候选并吸附
    const float edgesX[3] = {dragPrimaryStartX + dx, dragPrimaryStartX + dragPrimaryStartWidth * 0.5f + dx, dragPrimaryStartX + dragPrimaryStartWidth + dx};
    float bestX = threshold;
    float adjustX = 0.0f;
    float lineX = 0.0f;
    bool matchX = false;
    for (const float edge : edgesX) {
        for (const float candidate : candidatesX) {
            const float diff = std::abs(candidate - edge);
            if (diff < bestX) {
                bestX = diff;
                adjustX = candidate - edge;
                lineX = candidate;
                matchX = true;
            }
        }
    }
    if (matchX) {
        dx += adjustX;
        matchedLines.emplace_back('x', lineX);
    }

    const float edgesY[3] = {dragPrimaryStartY + dy, dragPrimaryStartY + dragPrimaryStartHeight * 0.5f + dy, dragPrimaryStartY + dragPrimaryStartHeight + dy};
    float bestY = threshold;
    float adjustY = 0.0f;
    float lineY = 0.0f;
    bool matchY = false;
    for (const float edge : edgesY) {
        for (const float candidate : candidatesY) {
            const float diff = std::abs(candidate - edge);
            if (diff < bestY) {
                bestY = diff;
                adjustY = candidate - edge;
                lineY = candidate;
                matchY = true;
            }
        }
    }
    if (matchY) {
        dy += adjustY;
        matchedLines.emplace_back('y', lineY);
    }
}

void ViewportPanel::showSmartLines(const std::vector<std::pair<char, float>>& lines) {
    if (!previewRoot)
        return;
    while (smartLines.size() < lines.size()) {
        auto line = MRColor::create();
        line->setColor(Vector4(0.95f, 0.25f, 0.85f, 0.9f));
        line->setDisplayLayer(9);
        previewRoot->addChild(line);
        smartLines.push_back(line);
    }
    for (size_t index = 0; index < smartLines.size(); ++index) {
        if (index < lines.size()) {
            const float thickness = 1.0f / std::max(0.2f, zoom);
            const Vector3 size = previewRoot->getTransform()->getSize();
            smartLines[index]->setVisible(true);
            if (lines[index].first == 'x') {
                smartLines[index]->getTransform()->setPosition(lines[index].second, 0.0f, 0.0f);
                smartLines[index]->getTransform()->setSize(thickness, size.y);
            } else {
                smartLines[index]->getTransform()->setPosition(0.0f, lines[index].second, 0.0f);
                smartLines[index]->getTransform()->setSize(size.x, thickness);
            }
        } else {
            smartLines[index]->setVisible(false);
        }
    }
}

void ViewportPanel::hideSmartLines() {
    for (const auto& line : smartLines)
        line->setVisible(false);
}

bool ViewportPanel::inTopRuler(float panelX, float panelY) const {
    return panelX >= kRulerThickness && panelY >= kViewportTopInset - kRulerThickness && panelY < kViewportTopInset;
}

bool ViewportPanel::inLeftRuler(float panelX, float panelY) const {
    return panelX >= 0.0f && panelX < kRulerThickness && panelY >= kViewportTopInset;
}

bool ViewportPanel::guideAt(float canvasX, float canvasY, bool& horizontal, float& value) const {
    const float threshold = kGuideHitPixels / std::max(0.2f, zoom);
    for (const float guide : m_shell.m_session->document().guideXs()) {
        if (std::abs(guide - canvasX) <= threshold) {
            horizontal = false;
            value = guide;
            return true;
        }
    }
    for (const float guide : m_shell.m_session->document().guideYs()) {
        if (std::abs(guide - canvasY) <= threshold) {
            horizontal = true;
            value = guide;
            return true;
        }
    }
    return false;
}

bool ViewportPanel::nodeAnchored(const std::string& nodeId) const {
    const auto* node = m_shell.m_session->document().findNode(nodeId);
    if (!node)
        return false;
    return node->properties.count("anchor_min") != 0 && node->properties.count("anchor_max") != 0;
}

bool ViewportPanel::nodeAnchorValues(const std::string& nodeId, float& minX, float& minY, float& maxX, float& maxY, float& parentWidth, float& parentHeight) const {
    const auto* node = m_shell.m_session->document().findNode(nodeId);
    if (!node)
        return false;
    if (node->properties.find("anchor_min") == node->properties.end() || node->properties.find("anchor_max") == node->properties.end())
        return false;
    if (!parseVector2Property(node, "anchor_min", minX, minY) || !parseVector2Property(node, "anchor_max", maxX, maxY))
        return false;
    // 父尺寸：文档父节点 size，根节点退回画布尺寸
    parentWidth = previewRoot ? previewRoot->getTransform()->getSize().x : 1280.0f;
    parentHeight = previewRoot ? previewRoot->getTransform()->getSize().y : 720.0f;
    if (!node->parentId.empty()) {
        float sizeX = 0.0f;
        float sizeY = 0.0f;
        const auto* parent = m_shell.m_session->document().findNode(node->parentId);
        if (parent && parseVector2Property(parent, "size", sizeX, sizeY) && sizeX > 0.0f) {
            parentWidth = sizeX;
            parentHeight = sizeY;
        }
    }
    return true;
}

bool ViewportPanel::applyAnchoredRect(const std::string& nodeId, float x, float y, float width, float height, bool continuous) {
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
    float parentWidth = 0.0f;
    float parentHeight = 0.0f;
    if (!nodeAnchorValues(nodeId, minX, minY, maxX, maxY, parentWidth, parentHeight))
        return false;
    const float left = x - minX * parentWidth;
    const float top = y - minY * parentHeight;
    const float right = maxX * parentWidth - (x + width);
    const float bottom = maxY * parentHeight - (y + height);
    char buffer[96];
    std::snprintf(buffer, sizeof(buffer), "Vector4(%.3f, %.3f, %.3f, %.3f)", left, top, right, bottom);
    std::string error;
    return m_shell.m_session->setProperty(nodeId, "anchor_offset", buffer, continuous, error);
}

void ViewportPanel::updateGuideDragPreview(float value) {
    if (!previewRoot)
        return;
    if (!guideDragPreview) {
        guideDragPreview = MRColor::create();
        guideDragPreview->setColor(Vector4(0.20f, 0.85f, 0.90f, 0.85f));
        guideDragPreview->setDisplayLayer(9);
        previewRoot->addChild(guideDragPreview);
    }
    if (guideDragPreview->m_parent != previewRoot)
        previewRoot->addChild(guideDragPreview);
    const float thickness = 1.0f / std::max(0.2f, zoom);
    const Vector3 size = previewRoot->getTransform()->getSize();
    guideDragPreview->setVisible(true);
    if (draggingGuideHorizontal) {
        guideDragPreview->getTransform()->setPosition(0.0f, value, 0.0f);
        guideDragPreview->getTransform()->setSize(size.x, thickness);
    } else {
        guideDragPreview->getTransform()->setPosition(value, 0.0f, 0.0f);
        guideDragPreview->getTransform()->setSize(thickness, size.y);
    }
}

void ViewportPanel::refreshGuides() {
    if (!previewRoot || !panel)
        return;

    // 画布叠加（网格 + 参考线）挂在 previewRoot 下，随 pan/zoom 变换
    for (const auto& line : gridLines)
        previewRoot->removeChild(line);
    for (const auto& line : guideLines)
        previewRoot->removeChild(line);
    for (const auto& bar : canvasBorders)
        previewRoot->removeChild(bar);
    for (const auto& bar : safeAreaBars)
        previewRoot->removeChild(bar);
    gridLines.clear();
    guideLines.clear();
    canvasBorders.clear();
    safeAreaBars.clear();

    const Vector3 previewSize = previewRoot->getTransform()->getSize();
    const float canvasLeft = -panX / zoom;
    const float canvasTop = -panY / zoom;
    const float canvasRight = (previewSize.x - panX) / zoom;
    const float canvasBottom = (previewSize.y - panY) / zoom;
    const float thickness = std::max(0.5f, 1.0f / zoom);

    if (snapEnabled) {
        // 自适应网格：屏幕间距不足 40px 时按 5 倍放大显示步长
        float visualStep = snapStep > 0.0f ? snapStep : 10.0f;
        while (visualStep * zoom < 40.0f)
            visualStep *= 5.0f;
        for (float value = std::ceil(canvasLeft / visualStep) * visualStep; value <= canvasRight; value += visualStep) {
            auto line = MRColor::create();
            line->setColor(Vector4(0.45f, 0.50f, 0.58f, 0.35f));
            line->setDisplayLayer(8);
            line->getTransform()->setPosition(value, canvasTop, 0.0f);
            line->getTransform()->setSize(thickness, canvasBottom - canvasTop);
            previewRoot->addChild(line);
            gridLines.push_back(line);
        }
        for (float value = std::ceil(canvasTop / visualStep) * visualStep; value <= canvasBottom; value += visualStep) {
            auto line = MRColor::create();
            line->setColor(Vector4(0.45f, 0.50f, 0.58f, 0.35f));
            line->setDisplayLayer(8);
            line->getTransform()->setPosition(canvasLeft, value, 0.0f);
            line->getTransform()->setSize(canvasRight - canvasLeft, thickness);
            previewRoot->addChild(line);
            gridLines.push_back(line);
        }
    }

    for (const float guide : m_shell.m_session->document().guideXs()) {
        auto line = MRColor::create();
        line->setColor(Vector4(0.20f, 0.85f, 0.90f, 0.8f));
        line->setDisplayLayer(8);
        line->getTransform()->setPosition(guide, canvasTop, 0.0f);
        line->getTransform()->setSize(thickness, canvasBottom - canvasTop);
        previewRoot->addChild(line);
        guideLines.push_back(line);
    }
    for (const float guide : m_shell.m_session->document().guideYs()) {
        auto line = MRColor::create();
        line->setColor(Vector4(0.20f, 0.85f, 0.90f, 0.8f));
        line->setDisplayLayer(8);
        line->getTransform()->setPosition(canvasLeft, guide, 0.0f);
        line->getTransform()->setSize(canvasRight - canvasLeft, thickness);
        previewRoot->addChild(line);
        guideLines.push_back(line);
    }

    if (canvasPreset > 0) {
        // 固定画布：白边框标示画布范围 + 安全区红条（左右 3%、上 6%、下 9% 模拟刘海/仪表遮挡）
        const auto addBar = [this](const Vector4& color, float x, float y, float w, float h, std::vector<std::shared_ptr<MRColor>>& target) {
            auto bar = MRColor::create();
            bar->setColor(color);
            bar->setDisplayLayer(12);
            bar->getTransform()->setPosition(x, y, 0.0f);
            bar->getTransform()->setSize(w, h);
            previewRoot->addChild(bar);
            target.push_back(std::move(bar));
        };
        const float borderThickness = 2.0f / std::max(0.05f, zoom);
        const Vector4 borderColor(0.92f, 0.95f, 1.0f, 0.85f);
        addBar(borderColor, 0.0f, 0.0f, previewSize.x, borderThickness, canvasBorders);
        addBar(borderColor, 0.0f, previewSize.y - borderThickness, previewSize.x, borderThickness, canvasBorders);
        addBar(borderColor, 0.0f, 0.0f, borderThickness, previewSize.y, canvasBorders);
        addBar(borderColor, previewSize.x - borderThickness, 0.0f, borderThickness, previewSize.y, canvasBorders);
        const Vector4 safeColor(0.95f, 0.25f, 0.30f, 0.15f);
        addBar(safeColor, 0.0f, 0.0f, previewSize.x * 0.03f, previewSize.y, safeAreaBars);
        addBar(safeColor, previewSize.x * 0.97f, 0.0f, previewSize.x * 0.03f, previewSize.y, safeAreaBars);
        addBar(safeColor, 0.0f, 0.0f, previewSize.x, previewSize.y * 0.06f, safeAreaBars);
        addBar(safeColor, 0.0f, previewSize.y * 0.91f, previewSize.x, previewSize.y * 0.09f, safeAreaBars);
    }

    rebuildRulers();
}

void ViewportPanel::rebuildRulers() {
    if (!panel)
        return;
    clearRulers();

    auto makeStrip = [this](float x, float y, float w, float h, const Vector4& color) {
        auto strip = MRColor::create();
        strip->setColor(color);
        strip->getTransform()->setPosition(x, y, 0.0f);
        strip->getTransform()->setSize(w, h);
        panel->addChild(strip);
        rulerBackgrounds.push_back(strip);
    };
    makeStrip(0.0f, kViewportTopInset - kRulerThickness, width, kRulerThickness, Vector4(0.09f, 0.11f, 0.14f, 1.0f));
    makeStrip(0.0f, kViewportTopInset, kRulerThickness, std::max(1.0f, height - kViewportTopInset), Vector4(0.09f, 0.11f, 0.14f, 1.0f));
    makeStrip(0.0f, kViewportTopInset - kRulerThickness, kRulerThickness, kRulerThickness, Vector4(0.07f, 0.09f, 0.12f, 1.0f));

    auto makeLabel = [this](const std::string& text, float x, float y, float w, float h) {
        auto label = std::make_shared<MRLabel>();
        label->setText(std::wstring(text.begin(), text.end()), "default");
        label->setFontSize(10.0f);
        label->setFontColor(0.55f, 0.60f, 0.68f, 1.0f);
        label->getTransform()->setPosition(x, y, 0.0f);
        label->getTransform()->setSize(w, h);
        panel->addChild(label);
        rulerLabels.push_back(label);
    };

    // 刻度步长：屏幕间距不足 80px 时按 5 倍放大
    float labelStep = snapStep > 0.0f ? snapStep : 10.0f;
    while (labelStep * zoom < 80.0f)
        labelStep *= 5.0f;
    size_t labelCount = 0;
    for (float value = std::ceil((-panX / zoom) / labelStep) * labelStep; value * zoom + panX <= width && labelCount < 64; value += labelStep, ++labelCount) {
        const float screenX = panX + value * zoom;
        if (screenX < kRulerThickness)
            continue;
        makeLabel(tickLabel(value), screenX - 24.0f, kViewportTopInset - kRulerThickness, 48.0f, kRulerThickness);
    }
    labelCount = 0;
    for (float value = std::ceil((-panY / zoom) / labelStep) * labelStep;
         kViewportTopInset + panY + value * zoom <= height && labelCount < 64; value += labelStep, ++labelCount) {
        const float screenY = kViewportTopInset + panY + value * zoom;
        if (screenY < kViewportTopInset)
            continue;
        makeLabel(tickLabel(value), 0.0f, screenY - 6.0f, kRulerThickness + 14.0f, 12.0f);
    }
}

void ViewportPanel::clearRulers() {
    if (!panel)
        return;
    for (const auto& strip : rulerBackgrounds)
        panel->removeChild(strip);
    for (const auto& label : rulerLabels)
        panel->removeChild(label);
    rulerBackgrounds.clear();
    rulerLabels.clear();
}

void ViewportPanel::resizeToPanel() {
    if (!panel)
        return;
    const Math::Rect bounds = panel->getWorldSpaceAABB();
    x = bounds.Min.x;
    y = bounds.Max.y;
    width = std::max(1.0f, bounds.GetWidth());
    height = std::max(1.0f, bounds.GetHeight());
    if (previewRoot) {
        if (canvasPreset > 0) {
            // 固定画布：面板变化后重新居中缩放
            applyCanvasPreset(canvasPreset);
            return;
        }
        previewRoot->getTransform()->setSize(width, std::max(1.0f, height - kViewportTopInset));
        refreshGuides();
        refreshSelectionOverlay();
    }
}

void ViewportPanel::focusSelection() {
    if (!previewRoot)
        return;
    if (canvasPreset > 0) {
        m_shell.setStatus("Focus is unavailable in fixed-canvas preview");
        return;
    }
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    bool any = false;
    for (const auto& nodeId : m_shell.m_session->model().selection().nodeIds) {
        float nodeX = 0.0f;
        float nodeY = 0.0f;
        float nodeWidth = 0.0f;
        float nodeHeight = 0.0f;
        if (!runtimeNodeRect(nodeId, nodeX, nodeY, nodeWidth, nodeHeight))
            continue;
        any = true;
        minX = std::min(minX, nodeX);
        minY = std::min(minY, nodeY);
        maxX = std::max(maxX, nodeX + nodeWidth);
        maxY = std::max(maxY, nodeY + nodeHeight);
    }
    if (!any) {
        m_shell.setStatus("Select a node to focus");
        return;
    }
    // 局部系 (panX, 32+panY) 为原点、zoom 缩放；使并集矩形居中并留边适配
    const Vector3 previewSize = previewRoot->getTransform()->getSize();
    const float margin = 40.0f;
    const float rectWidth = std::max(1.0f, maxX - minX);
    const float rectHeight = std::max(1.0f, maxY - minY);
    const float fitZoom = std::min((previewSize.x - margin * 2.0f) / rectWidth, (previewSize.y - margin * 2.0f) / rectHeight);
    zoom = std::clamp(std::min(fitZoom, 5.0f), 0.2f, 5.0f);
    panX = previewSize.x * 0.5f - (minX + maxX) * 0.5f * zoom;
    panY = previewSize.y * 0.5f - (minY + maxY) * 0.5f * zoom;
    previewRoot->getTransform()->setPosition(panX, kViewportTopInset + panY, 0.0f);
    previewRoot->getTransform()->setScale(zoom, zoom, 1.0f);
    refreshGuides();
    refreshSelectionOverlay();
    m_shell.setStatus("Focused selection");
}

int ViewportPanel::currentCanvasPreset() const {
    return canvasPreset;
}

void ViewportPanel::applyCanvasPreset(int presetIndex) {
    if (!previewRoot || !panel)
        return;
    if (presetIndex < 0 || presetIndex >= kCanvasPresetCount)
        presetIndex = 0;
    canvasPreset = presetIndex;
    const Math::Rect bounds = panel->getWorldSpaceAABB();
    const float panelWidth = std::max(1.0f, bounds.GetWidth());
    const float panelHeight = std::max(1.0f, bounds.GetHeight() - kViewportTopInset);
    if (presetIndex == 0) {
        // Fit：恢复面板自适应
        zoom = 1.0f;
        panX = 0.0f;
        panY = 0.0f;
        previewRoot->getTransform()->setSize(panelWidth, panelHeight);
        previewRoot->getTransform()->setPosition(0.0f, kViewportTopInset, 0.0f);
        previewRoot->getTransform()->setScale(1.0f, 1.0f, 1.0f);
        m_shell.setStatus("Canvas: fit to panel");
    } else {
        // 固定分辨率：previewRoot 改为预设尺寸并整体缩放居中，
        // 容器与锚点在真实画布尺寸下重排
        const float canvasWidth = kCanvasPresets[presetIndex].width;
        const float canvasHeight = kCanvasPresets[presetIndex].height;
        zoom = std::min((panelWidth - 16.0f) / canvasWidth, (panelHeight - 16.0f) / canvasHeight);
        zoom = std::clamp(zoom, 0.05f, 1.0f);
        previewRoot->getTransform()->setSize(canvasWidth, canvasHeight);
        panX = (panelWidth - canvasWidth * zoom) * 0.5f;
        panY = (panelHeight - canvasHeight * zoom) * 0.5f;
        previewRoot->getTransform()->setPosition(panX, kViewportTopInset + panY, 0.0f);
        previewRoot->getTransform()->setScale(zoom, zoom, 1.0f);
        m_shell.setStatus(std::string("Canvas: ") + kCanvasPresets[presetIndex].name + " (anchors/containers relayout)");
    }
    refreshGuides();
    refreshSelectionOverlay();
}

void ViewportPanel::handlePointer(const TouchEvent& event) {
    const Math::Rect viewportBounds = panel->getWorldSpaceAABB();
    x = viewportBounds.Min.x;
    y = viewportBounds.Max.y;
    width = viewportBounds.GetWidth();
    height = viewportBounds.GetHeight();
    // 世界系 Y 向上：panelX 从左缘向右量，panelY 从顶缘（Max.y）向下量，
    // 与 32px 顶栏偏移、pan/zoom 公式的“左上原点 Y 向下”语义一致。
    // 注意：局部画布坐标命名 canvasX/canvasY，避免遮蔽成员 x/y（遮蔽会
    // 破坏本函数内对 panel->getWorldSpaceAABB() 等成员访问的名称查找）。
    const float panelX = event.positionX - x;
    const float panelY = y - event.positionY;
    const float canvasX = (panelX - panX) / zoom;
    const float canvasY = (panelY - kViewportTopInset - panY) / zoom;

    // ---- 参考线交互（从标尺拖出新线 / 拖动已有线；拖回标尺区删除） ----
    if (event.eventType == TOUCH_EVENT_TYPE_TOUCH && event.button == TOUCH_MOUSE_BUTTON_LEFT && panelX >= 0.0f && panelY >= kViewportTopInset - kRulerThickness &&
        panelX <= width && panelY <= height) {
        bool horizontal = false;
        float value = 0.0f;
        if (guideAt(canvasX, canvasY, horizontal, value)) {
            // 抓住已有参考线：先从文档移除，落点决定重建或删除
            auto& document = m_shell.m_session->document();
            if (horizontal)
                document.removeGuideY(value);
            else
                document.removeGuideX(value);
            m_shell.m_session->markDirty();
            draggingGuide = true;
            draggingGuideHorizontal = horizontal;
            draggingExistingGuide = true;
            updateGuideDragPreview(snapValue(horizontal ? canvasY : canvasX));
            refreshGuides();
            return;
        }
        if (inTopRuler(panelX, panelY) || inLeftRuler(panelX, panelY)) {
            draggingGuide = true;
            draggingGuideHorizontal = inLeftRuler(panelX, panelY);
            draggingExistingGuide = false;
            updateGuideDragPreview(snapValue(draggingGuideHorizontal ? canvasY : canvasX));
            return;
        }
    }
    if (event.eventType == TOUCH_EVENT_TYPE_MOVE && draggingGuide) {
        updateGuideDragPreview(snapValue(draggingGuideHorizontal ? canvasY : canvasX));
        return;
    }
    if (event.eventType == TOUCH_EVENT_TYPE_RELEASE && draggingGuide) {
        draggingGuide = false;
        if (guideDragPreview && guideDragPreview->m_parent == previewRoot)
            previewRoot->removeChild(guideDragPreview);
        guideDragPreview.reset();
        const float value = draggingGuideHorizontal ? snapValue(canvasY) : snapValue(canvasX);
        auto& document = m_shell.m_session->document();
        if (inTopRuler(panelX, panelY) || inLeftRuler(panelX, panelY)) {
            m_shell.setStatus(draggingExistingGuide ? "Guide removed" : "Guide cancelled");
        } else if (draggingGuideHorizontal) {
            document.addGuideY(value);
            m_shell.m_session->markDirty();
            m_shell.setStatus("Guide added (horizontal)");
        } else {
            document.addGuideX(value);
            m_shell.m_session->markDirty();
            m_shell.setStatus("Guide added (vertical)");
        }
        refreshGuides();
        return;
    }

    if (panelX < 0.0f || panelY < kViewportTopInset || panelX > width || panelY > height)
        return;
    if (canvasPreset > 0) {
        // 固定画布模式下保持 fit 视图：禁用滚轮缩放与平移（预览分辨率由预设决定）
        if (event.eventType == TOUCH_EVENT_TYPE_WHEEL || (event.eventType == TOUCH_EVENT_TYPE_TOUCH && event.button == TOUCH_MOUSE_BUTTON_MIDDLE))
            return;
    }
    if (event.eventType == TOUCH_EVENT_TYPE_WHEEL) {
        const float oldZoom = zoom;
        zoom = std::clamp(zoom * (event.wheelDeltaY > 0.0f ? 1.1f : 0.9f), 0.2f, 5.0f);
        panX = panelX - canvasX * zoom;
        panY = panelY - kViewportTopInset - canvasY * zoom;
        previewRoot->getTransform()->setPosition(panX, kViewportTopInset + panY, 0.0f);
        previewRoot->getTransform()->setScale(zoom, zoom, 1.0f);
        refreshGuides();
        refreshSelectionOverlay();
        m_shell.setStatus("Viewport zoom " + std::to_string(zoom) + " (was " + std::to_string(oldZoom) + ")");
        return;
    }
    if (event.eventType == TOUCH_EVENT_TYPE_TOUCH && event.button == TOUCH_MOUSE_BUTTON_MIDDLE) {
        panning = true;
        lastPointerX = panelX;
        lastPointerY = panelY;
        return;
    }
    if (event.eventType == TOUCH_EVENT_TYPE_MOVE && panning) {
        panX += panelX - lastPointerX;
        panY += panelY - lastPointerY;
        lastPointerX = panelX;
        lastPointerY = panelY;
        previewRoot->getTransform()->setPosition(panX, kViewportTopInset + panY, 0.0f);
        refreshGuides();
        return;
    }
    if (event.eventType == TOUCH_EVENT_TYPE_TOUCH && event.button == TOUCH_MOUSE_BUTTON_LEFT) {
        float selectedX = 0.0f;
        float selectedY = 0.0f;
        float selectedWidth = 0.0f;
        float selectedHeight = 0.0f;
        if (selectedRuntimeRect(selectedX, selectedY, selectedWidth, selectedHeight)) {
            const ResizeHandle handle = resizeHandleAt(canvasX, canvasY, selectedX, selectedY, selectedWidth, selectedHeight);
            if (handle != ResizeHandle::None) {
                if (m_shell.m_session->model().selectedLocalRect(resizeNodeStartX, resizeNodeStartY, resizeNodeStartZ, resizeNodeStartWidth, resizeNodeStartHeight)) {
                    resizeHandle = handle;
                    resizing = true;
                    dragging = false;
                    transformChanged = false;
                    resizePointerStartX = canvasX;
                    resizePointerStartY = canvasY;
                    resizeAnchored = nodeAnchored(m_shell.m_selectedNodeId);
                    return;
                }
            }
            if (rotationHandleAt(canvasX, canvasY, selectedX, selectedY, selectedWidth, selectedHeight)) {
                const auto* node = m_shell.m_session->document().findNode(m_shell.m_selectedNodeId);
                if (!parseVector3Property(node, "rotation", rotateStartRotationZ, rotateStartRotationZ, rotateStartRotationZ)) {
                    m_shell.setStatus("Node rotation property is malformed");
                    return;
                }
                rotating = true;
                dragging = false;
                resizing = false;
                resizeHandle = ResizeHandle::None;
                transformChanged = false;
                rotatePivotX = selectedX + selectedWidth * 0.5f;
                rotatePivotY = selectedY + selectedHeight * 0.5f;
                rotateStartPointerAngle = std::atan2(canvasY - rotatePivotY, canvasX - rotatePivotX);
                return;
            }
        }
        const bool additive = (event.modifiers & TOUCH_MODIFIER_CTRL) != 0;
        const std::string targetNodeId = runtimeNodeAt(canvasX, canvasY);
        std::string screenTargetNodeId = targetNodeId;
        if (screenTargetNodeId.empty()) {
            for (const auto& [nodeId, runtimeNode] : runtimeNodes) {
                if (m_shell.m_session->document().isNodeLocked(nodeId))
                    continue;
                const auto uiNode = std::dynamic_pointer_cast<UIWidget>(runtimeNode);
                if (uiNode && uiNode->getVisible() && uiNode->getWorldSpaceAABB().Contains(event.positionX, event.positionY)) {
                    screenTargetNodeId = nodeId;
                    break;
                }
            }
        }
        std::string error;
        if (!screenTargetNodeId.empty() && m_shell.m_session->selectNode(screenTargetNodeId, additive, error)) {
            m_shell.m_inspector->clearAsset();
            const bool selectionChanged = m_shell.notifySelectionChanged();
            if (m_shell.m_leftTabs)
                m_shell.m_leftTabs->selectTab("scene_tree");
            resizing = false;
            resizeHandle = ResizeHandle::None;
            dragging = true;
            transformChanged = false;
            lastPointerX = panelX;
            lastPointerY = panelY;
            // 吸附基准：主选中节点（最后选中）的文档局部 position/size
            const auto* primaryNode = m_shell.m_session->document().findNode(m_shell.m_selectedNodeId);
            if (primaryNode && parseVector3Property(primaryNode, "position", dragPrimaryStartX, dragPrimaryStartY, dragPrimaryStartHeight)) {
                parseVector2Property(primaryNode, "size", dragPrimaryStartWidth, dragPrimaryStartHeight);
            } else {
                dragPrimaryStartX = 0.0f;
                dragPrimaryStartY = 0.0f;
            }
            dragStartPointerX = panelX;
            dragStartPointerY = panelY;
            dragAppliedDx = 0.0f;
            dragAppliedDy = 0.0f;
            // 单选锚定节点：拖拽/缩放改写 anchor_offset 而非 position/size
            dragAnchored = nodeAnchored(m_shell.m_selectedNodeId) && m_shell.m_session->model().selection().nodeIds.size() == 1;
            if (selectionChanged)
                m_shell.setStatus("Selected " + screenTargetNodeId);
        } else if (screenTargetNodeId.empty()) {
            // 空白处按下：开始橡皮筋框选；普通左键先清空选择（与 Godot 一致）
            rubberAdditive = additive;
            if (additive) {
                rubberBaseSelection = m_shell.m_session->model().selection().nodeIds;
            } else {
                rubberBaseSelection.clear();
                m_shell.m_session->model().clearSelection();
                m_shell.m_selectedNodeId.clear();
                m_shell.notifySelectionChanged();
            }
            rubberBanding = true;
            dragging = false;
            transformChanged = false;
            rubberStartX = canvasX;
            rubberStartY = canvasY;
            rubberCurrentX = canvasX;
            rubberCurrentY = canvasY;
            updateRubberBandOverlay();
        }
    } else if (event.eventType == TOUCH_EVENT_TYPE_MOVE && resizing && !m_shell.m_selectedNodeId.empty()) {
        const bool snap = snapEnabled && (event.modifiers & TOUCH_MODIFIER_CTRL) == 0;
        const float dx = snap ? snapValue(canvasX) - snapValue(resizePointerStartX) : canvasX - resizePointerStartX;
        const float dy = snap ? snapValue(canvasY) - snapValue(resizePointerStartY) : canvasY - resizePointerStartY;
        float nodeX = resizeNodeStartX;
        float nodeY = resizeNodeStartY;
        float nodeWidth = resizeNodeStartWidth;
        float nodeHeight = resizeNodeStartHeight;
        const bool left = resizeHandle == ResizeHandle::Left || resizeHandle == ResizeHandle::TopLeft || resizeHandle == ResizeHandle::BottomLeft;
        const bool right = resizeHandle == ResizeHandle::Right || resizeHandle == ResizeHandle::TopRight || resizeHandle == ResizeHandle::BottomRight;
        const bool top = resizeHandle == ResizeHandle::Top || resizeHandle == ResizeHandle::TopLeft || resizeHandle == ResizeHandle::TopRight;
        const bool bottom = resizeHandle == ResizeHandle::Bottom || resizeHandle == ResizeHandle::BottomLeft || resizeHandle == ResizeHandle::BottomRight;
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
        if (resizeAnchored) {
            // 锚定节点：新矩形换算为 anchor_offset（保持锚点），position/size 由锚点推导
            if (applyAnchoredRect(m_shell.m_selectedNodeId, nodeX, nodeY, nodeWidth, nodeHeight, true)) {
                syncRuntimeNode(m_shell.m_selectedNodeId, false);
                refreshSelectionOverlay();
                transformChanged = true;
            }
            return;
        }
        std::string error;
        if (m_shell.m_session->setNodeRect(m_shell.m_selectedNodeId, nodeX, nodeY, resizeNodeStartZ, nodeWidth, nodeHeight, true, error)) {
            syncRuntimeNode(m_shell.m_selectedNodeId, true);
            refreshSelectionOverlay();
            transformChanged = true;
        }
    } else if (event.eventType == TOUCH_EVENT_TYPE_MOVE && rotating && !m_shell.m_selectedNodeId.empty()) {
        const float angle = std::atan2(canvasY - rotatePivotY, canvasX - rotatePivotX);
        // 局部系 y 向下：atan2 随顺时针增大，而 rotation z 正向为逆时针，故增量取负
        constexpr float kPi = 3.14159265359f;
        float delta = angle - rotateStartPointerAngle;
        if (delta > kPi)
            delta -= 2.0f * kPi;
        else if (delta < -kPi)
            delta += 2.0f * kPi;
        float newZ = rotateStartRotationZ - delta * 180.0f / kPi;
        if ((event.modifiers & TOUCH_MODIFIER_SHIFT) != 0)
            newZ = std::round(newZ / 15.0f) * 15.0f;
        std::string error;
        if (m_shell.m_session->setProperty(m_shell.m_selectedNodeId, "rotation", rotationZValue(newZ), true, error)) {
            syncRuntimeNode(m_shell.m_selectedNodeId, true);
            refreshSelectionOverlay();
            transformChanged = true;
        }
    } else if (event.eventType == TOUCH_EVENT_TYPE_MOVE && dragging && !m_shell.m_selectedNodeId.empty()) {
        // 绝对位移协议：由手势起点算总位移，吸附后换算成本次增量下发，命令合并语义保持增量累加
        const float rawDx = (panelX - dragStartPointerX) / zoom;
        const float rawDy = (panelY - dragStartPointerY) / zoom;
        float dx = rawDx;
        float dy = rawDy;
        std::vector<std::pair<char, float>> matchedLines;
        if (snapEnabled && (event.modifiers & TOUCH_MODIFIER_CTRL) == 0) {
            applyGridSnapToDelta(dx, dy);
            applySmartSnapToDelta(dx, dy, matchedLines);
        }
        if (!matchedLines.empty())
            showSmartLines(matchedLines);
        else
            hideSmartLines();
        if (dragAnchored) {
            // 锚定单节点：吸附后的目标矩形换算 anchor_offset 写回（保持锚点不变）
            if (applyAnchoredRect(m_shell.m_selectedNodeId, dragPrimaryStartX + dx, dragPrimaryStartY + dy, dragPrimaryStartWidth, dragPrimaryStartHeight, true)) {
                syncRuntimeNode(m_shell.m_selectedNodeId, false);
                refreshSelectionOverlay();
                transformChanged = true;
            }
            lastPointerX = panelX;
            lastPointerY = panelY;
            return;
        }
        const float incDx = dx - dragAppliedDx;
        const float incDy = dy - dragAppliedDy;
        if (incDx != 0.0f || incDy != 0.0f) {
            std::string error;
            if (m_shell.m_session->moveSelection(incDx, incDy, true, error)) {
                syncSelectedRuntimeNodes(true);
                refreshSelectionOverlay();
                transformChanged = true;
                dragAppliedDx = dx;
                dragAppliedDy = dy;
            }
        }
        lastPointerX = panelX;
        lastPointerY = panelY;
    } else if (event.eventType == TOUCH_EVENT_TYPE_MOVE && rubberBanding) {
        rubberCurrentX = canvasX;
        rubberCurrentY = canvasY;
        updateRubberBandOverlay();
    } else if (event.eventType == TOUCH_EVENT_TYPE_RELEASE) {
        if (rubberBanding) {
            rubberCurrentX = canvasX;
            rubberCurrentY = canvasY;
            rubberBanding = false;
            updateRubberBandOverlay();
            finishRubberBandSelection();
        }
        hideSmartLines();
        const bool changed = transformChanged;
        dragging = false;
        resizing = false;
        rotating = false;
        resizeHandle = ResizeHandle::None;
        panning = false;
        transformChanged = false;
        if (changed)
            m_shell.m_inspector->refresh();
    }
}

}  // namespace morrow::editor
