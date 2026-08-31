#include "EditorShell.h"
#include "panels/InspectorPanel.h"
#include "panels/ViewportPanel.h"

#include <algorithm>
#include <array>
#include <limits>

#include "base/Interaction.h"
#include "base/Transform.h"
#include "base/TouchEvent.h"
#include "elements/MRColor.h"
#include "scene/SceneInstantiator.h"

namespace morrow::editor {

ViewportPanel::ViewportPanel(EditorShell& shell) : m_shell(shell) {
}

void ViewportPanel::rebuildRuntime() {
    if (!previewRoot)
        return;
    for (const auto& border : selectionBorders)
        previewRoot->removeChild(border);
    for (const auto& handle : selectionHandles)
        previewRoot->removeChild(handle);
    previewRoot->m_children.clear();
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
        return;
    }

    const Vector4 borderColor(1.0f, 0.34f, 0.18f, 1.0f);
    while (selectionBorders.size() < 4) {
        auto border = MRColor::create();
        border->setColor(borderColor);
        border->setDisplayLayer(10);
        selectionBorders.push_back(border);
    }
    while (selectionHandles.size() < 8) {
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
    const std::vector<Vector2> points = {
        {x, y},          {x + width * 0.5f, y},  {x + width, y}, {x + width, y + height * 0.5f}, {x + width, y + height}, {x + width * 0.5f, y + height},
        {x, y + height}, {x, y + height * 0.5f},
    };
    for (size_t index = 0; index < points.size(); ++index) {
        auto handle = std::dynamic_pointer_cast<MRColor>(selectionHandles[index]);
        if (handle)
            handle->setRounding(handleSize * 0.5f);
        selectionHandles[index]->getTransform()->setPosition(points[index].x - handleSize * 0.5f, points[index].y - handleSize * 0.5f, 0.0f);
        selectionHandles[index]->getTransform()->setSize(handleSize, handleSize);
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
    const auto& nodes = m_shell.m_session->document().nodes();
    for (auto node = nodes.rbegin(); node != nodes.rend(); ++node) {
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

void ViewportPanel::refreshGuides() {
    if (!previewRoot)
        return;

    if (previewCanvas) {
        previewRoot->removeChild(previewCanvas);
        previewCanvas.reset();
    }
    if (previewGrid) {
        previewRoot->removeChild(previewGrid);
        previewGrid.reset();
    }
}

void ViewportPanel::handlePointer(const TouchEvent& event) {
    const Math::Rect viewportBounds = panel->getScreenSpaceAABB();
    x = viewportBounds.Min.x;
    y = viewportBounds.Min.y;
    width = viewportBounds.GetWidth();
    height = viewportBounds.GetHeight();
    const float panelX = event.positionX - x;
    const float panelY = event.positionY - y;
    const float x = (panelX - panX) / zoom;
    const float y = (panelY - 32.0f - panY) / zoom;
    if (panelX < 0.0f || panelY < 32.0f || panelX > width || panelY > height)
        return;
    if (event.eventType == TOUCH_EVENT_TYPE_WHEEL) {
        const float oldZoom = zoom;
        zoom = std::clamp(zoom * (event.wheelDeltaY > 0.0f ? 1.1f : 0.9f), 0.2f, 5.0f);
        panX = panelX - x * zoom;
        panY = panelY - 32.0f - y * zoom;
        previewRoot->getTransform()->setPosition(panX, 32.0f + panY, 0.0f);
        previewRoot->getTransform()->setScale(zoom, zoom, 1.0f);
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
        previewRoot->getTransform()->setPosition(panX, 32.0f + panY, 0.0f);
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
                if (m_shell.m_session->model().selectedLocalRect(resizeNodeStartX, resizeNodeStartY, resizeNodeStartZ, resizeNodeStartWidth, resizeNodeStartHeight)) {
                    resizeHandle = handle;
                    resizing = true;
                    dragging = false;
                    transformChanged = false;
                    resizePointerStartX = x;
                    resizePointerStartY = y;
                    return;
                }
            }
        }
        const bool additive = (event.modifiers & TOUCH_MODIFIER_CTRL) != 0;
        const std::string targetNodeId = runtimeNodeAt(x, y);
        std::string error;
        if (!targetNodeId.empty() && m_shell.m_session->selectNode(targetNodeId, additive, error)) {
            m_shell.notifySelectionChanged();
            if (m_shell.m_leftTabs)
                m_shell.m_leftTabs->selectTab("scene_tree");
            resizing = false;
            resizeHandle = ResizeHandle::None;
            dragging = true;
            transformChanged = false;
            lastPointerX = panelX;
            lastPointerY = panelY;
            m_shell.setStatus("Selected " + targetNodeId);
        }
    } else if (event.eventType == TOUCH_EVENT_TYPE_MOVE && resizing && !m_shell.m_selectedNodeId.empty()) {
        const float dx = x - resizePointerStartX;
        const float dy = y - resizePointerStartY;
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
        std::string error;
        if (m_shell.m_session->setNodeRect(m_shell.m_selectedNodeId, nodeX, nodeY, resizeNodeStartZ, nodeWidth, nodeHeight, true, error)) {
            syncRuntimeNode(m_shell.m_selectedNodeId, true);
            refreshSelectionOverlay();
            transformChanged = true;
        }
    } else if (event.eventType == TOUCH_EVENT_TYPE_MOVE && dragging && !m_shell.m_selectedNodeId.empty()) {
        float dx = (panelX - lastPointerX) / zoom;
        float dy = (panelY - lastPointerY) / zoom;
        std::string error;
        if (m_shell.m_session->moveSelection(dx, dy, true, error)) {
            syncSelectedRuntimeNodes(true);
            refreshSelectionOverlay();
            transformChanged = true;
            lastPointerX = panelX;
            lastPointerY = panelY;
        }
    } else if (event.eventType == TOUCH_EVENT_TYPE_RELEASE) {
        const bool changed = transformChanged;
        dragging = false;
        resizing = false;
        resizeHandle = ResizeHandle::None;
        panning = false;
        transformChanged = false;
        if (changed)
            m_shell.m_inspector->refresh();
    }
}

}  // namespace morrow::editor
