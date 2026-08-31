#include "panels/SceneTreePanel.h"

#include <algorithm>
#include <codecvt>
#include <locale>

#include "EditorShell.h"
#include "Engine.h"
#include "base/Interaction.h"
#include "base/TouchEvent.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "elements/MRLineEdit.h"
#include "elements/MRPopupMenu.h"
#include "panels/InspectorPanel.h"
#include "panels/ViewportPanel.h"

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
}  // namespace

namespace morrow::editor {

SceneTreePanel::SceneTreePanel(EditorShell& shell) : m_shell(shell) {
}

void SceneTreePanel::cancelRename() {
    if (!renameEdit)
        return;
    renameEdit->onFocusChanged(false);
    if (panel)
        panel->removeChild(renameEdit);
    for (auto& row : rows) {
        if (row.nodeId == renameNodeId && row.button) {
            row.button->setVisible(true);
            break;
        }
    }
    renameEdit.reset();
    renameNodeId.clear();
}

void SceneTreePanel::refresh() {
    if (!panel)
        return;
    cancelRename();
    while (panel->m_children.size() > 2) {
        panel->m_children.pop_back();
    }
    contextConnections.clear();
    rows.clear();
    float y = 38.0f;
    const float panelWidth = std::max(80.0f, panel->getTransform()->getSize().x);
    for (const auto& item : m_shell.m_session->model().buildSceneTree()) {
        const auto id = item.id;
        auto button = m_shell.addButton(panel, std::wstring(item.depth * 2, L' ') + wide(item.name), 8.0f + item.depth * 12.0f, y,
                                        std::max(40.0f, panelWidth - 16.0f - item.depth * 12.0f), 30.0f, [this, id] {
                                            std::string error;
                                            if (m_shell.m_session->selectNode(id, false, error)) {
                                                m_shell.m_inspector->clearAsset();
                                                m_shell.notifySelectionChanged();
                                                m_shell.setStatus("Selected " + id);
                                            } else
                                                m_shell.setStatus(error);
                                        });
        button->setWidgetName("SceneTree_" + item.id);
        button->setTextAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
        rows.push_back({item.id, button});
        if (auto interaction = button->getComponent<Interaction>()) {
            const auto parentId = item.id;
            contextConnections.emplace_back(interaction->addEventListener(
                TOUCH_EVENT_TYPE_TOUCH,
                [this, parentId](TouchEvent& event) {
                    if (event.button != TOUCH_MOUSE_BUTTON_RIGHT)
                        return;
                    std::string error;
                    if (m_shell.m_session->selectNode(parentId, false, error))
                        m_shell.notifySelectionChanged();
                    contextParentId = parentId;
                    contextMenu->attachTo(m_shell.m_shellRoot);
                    contextMenu->popup(event.positionX, event.positionY);
                },
                20));
        }
        y += 32.0f;
    }
    refreshSelectionStyles();
}

void SceneTreePanel::refreshSelectionStyles() {
    const auto& selected = m_shell.m_session->model().selection().nodeIds;
    for (auto& row : rows) {
        if (!row.button)
            continue;
        const bool isSelected = std::find(selected.begin(), selected.end(), row.nodeId) != selected.end();
        const bool isDropTarget = dragging && row.nodeId == dropTarget;
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

std::string SceneTreePanel::nodeForWidget(const std::shared_ptr<Widget>& widget) const {
    if (!widget)
        return {};
    for (const auto& row : rows) {
        if (row.button == widget)
            return row.nodeId;
    }
    return {};
}

std::string SceneTreePanel::nodeAt(float x, float y) const {
    for (const auto& row : rows) {
        if (row.button && row.button->getScreenSpaceAABB().Contains(x, y)) {
            return row.nodeId;
        }
    }
    return {};
}

bool SceneTreePanel::handleDrag(const TouchEvent& event) {
    if (event.eventType == TOUCH_EVENT_TYPE_TOUCH && event.button == TOUCH_MOUSE_BUTTON_LEFT) {
        const std::string nodeId = nodeForWidget(event.target);
        if (nodeId.empty())
            return false;
        pendingDragNode = nodeId;
        dragStartX = event.positionX;
        dragStartY = event.positionY;
        return false;
    }

    if (event.eventType == TOUCH_EVENT_TYPE_MOVE) {
        if (!dragging && !pendingDragNode.empty()) {
            const float dx = event.positionX - dragStartX;
            const float dy = event.positionY - dragStartY;
            if (dx * dx + dy * dy >= 36.0f) {
                dragging = true;
                dragNode = pendingDragNode;
                std::string error;
                if (m_shell.m_session->selectNode(dragNode, false, error)) {
                    m_shell.notifySelectionChanged();
                }
            }
        }
        if (!dragging)
            return false;
        std::string target = nodeAt(event.positionX, event.positionY);
        if (target == dragNode)
            target.clear();
        if (dropTarget != target) {
            dropTarget = std::move(target);
            refreshSelectionStyles();
        }
        return true;
    }

    if (event.eventType == TOUCH_EVENT_TYPE_RELEASE) {
        pendingDragNode.clear();
        if (!dragging)
            return false;
        const std::string dragged = dragNode;
        const std::string target = dropTarget;
        dragging = false;
        dragNode.clear();
        dropTarget.clear();
        refreshSelectionStyles();
        if (target.empty())
            return true;
        const auto* node = m_shell.m_session->document().findNode(dragged);
        if (node && node->parentId == target) {
            m_shell.setStatus("Node already has the requested parent");
            return true;
        }
        std::string error;
        if (!m_shell.m_session->reparentNode(dragged, target, error)) {
            m_shell.setStatus("Cannot reparent node: " + error);
            return true;
        }
        refresh();
        m_shell.m_viewport->rebuildRuntime();
        m_shell.m_inspector->refresh();
        m_shell.setStatus("Reparented " + dragged + " under " + target);
        return true;
    }
    return false;
}

void SceneTreePanel::deleteSelected() {
    const auto selected = m_shell.m_session->model().selection().nodeIds;
    if (selected.empty()) {
        m_shell.setStatus("Select a scene node to delete");
        return;
    }
    const std::string nodeId = selected.back();
    const auto* node = m_shell.m_session->document().findNode(nodeId);
    if (!node) {
        m_shell.setStatus("Selected scene node no longer exists");
        return;
    }
    if (node->parentId.empty()) {
        m_shell.setStatus("The scene root node cannot be deleted");
        return;
    }
    std::string error;
    if (!m_shell.m_session->deleteNode(nodeId, error)) {
        m_shell.setStatus("Failed to delete node: " + error);
        return;
    }
    m_shell.m_session->model().clearSelection();
    m_shell.m_selectedNodeId.clear();
    m_shell.notifySelectionChanged();
    refresh();
    m_shell.m_viewport->rebuildRuntime();
    m_shell.m_inspector->refresh();
    m_shell.setStatus("Deleted node " + nodeId);
}

void SceneTreePanel::beginRename(const std::string& nodeId) {
    std::string resolved = nodeId;
    if (resolved.empty() && !m_shell.m_session->model().selection().nodeIds.empty()) {
        resolved = m_shell.m_session->model().selection().nodeIds.back();
    }
    const auto* node = m_shell.m_session->document().findNode(resolved);
    if (!node) {
        m_shell.setStatus("Select a scene node to rename");
        return;
    }
    cancelRename();
    const auto row = std::find_if(rows.begin(), rows.end(), [&resolved](const SceneTreeRow& candidate) { return candidate.nodeId == resolved; });
    if (row == rows.end() || !row->button) {
        m_shell.setStatus("Scene node row is not available");
        return;
    }

    const Vector3 position = row->button->getTransform()->getPosition();
    const Vector3 size = row->button->getTransform()->getSize();
    row->button->setVisible(false);
    renameNodeId = resolved;
    renameEdit = MRLineEdit::create();
    renameEdit->setText(wide(node->name));
    renameEdit->setFontSize(16.0f);
    renameEdit->setTextColor(Vector4(0.98f, 0.99f, 1.0f, 1.0f));
    renameEdit->setBackgroundColor(Vector4(0.08f, 0.18f, 0.34f, 1.0f));
    renameEdit->setFocusedBackgroundColor(Vector4(0.10f, 0.24f, 0.46f, 1.0f));
    if (auto interaction = renameEdit->getComponent<Interaction>()) {
        interaction->setKeyboardFocusable(false);
    }
    renameEdit->getTransform()->setPosition(position);
    renameEdit->getTransform()->setSize(size);
    panel->addChild(renameEdit);
    renameEdit->onFocusChanged(true);
    renameEdit->selectAll();
}

void SceneTreePanel::commitRename() {
    if (!renameEdit || renameNodeId.empty())
        return;
    const std::string nodeId = renameNodeId;
    const std::string name = narrow(renameEdit->getText());
    if (name.empty()) {
        m_shell.setStatus("Node name cannot be empty");
        return;
    }
    renameNode(nodeId, name);
}

void SceneTreePanel::renameNode(const std::string& nodeId, const std::string& name) {
    std::string error;
    if (!m_shell.m_session->renameNode(nodeId, name, error)) {
        m_shell.setStatus("Failed to rename node: " + error);
        return;
    }
    refresh();
    m_shell.m_viewport->syncRuntimeNode(nodeId, false);
    m_shell.m_viewport->refreshSelectionOverlay();
    m_shell.m_inspector->refresh();
    m_shell.setStatus("Renamed node " + nodeId + " to " + name);
}

void SceneTreePanel::showCreateDialog(const std::string& parentId) {
    std::string resolvedParent = parentId;
    if (resolvedParent.empty() && !m_shell.m_session->model().selection().nodeIds.empty()) {
        resolvedParent = m_shell.m_session->model().selection().nodeIds.back();
    }
    if (resolvedParent.empty()) {
        for (const auto& node : m_shell.m_session->document().nodes()) {
            if (node.parentId.empty()) {
                resolvedParent = node.id;
                break;
            }
        }
    }
    const auto* parent = m_shell.m_session->document().findNode(resolvedParent);
    if (!parent) {
        m_shell.setStatus("Select a valid parent node first");
        return;
    }
    createDialog->showForParent(parent->id, parent->name);
}

void SceneTreePanel::createChild(const NodeTypeDescriptor& descriptor, const std::string& parentId) {
    SceneNodeRecord node = m_shell.m_nodeTypeCatalog.createNode(descriptor, parentId, m_shell.m_session->document());
    const std::string nodeId = node.id;
    const std::string nodeName = node.name;
    std::string error;
    if (!m_shell.m_session->addNode(std::move(node), error)) {
        m_shell.setStatus("Failed to create node: " + error);
        return;
    }
    if (!m_shell.m_session->selectNode(nodeId, false, error)) {
        m_shell.setStatus("Node created, but selection failed: " + error);
    } else {
        m_shell.notifySelectionChanged();
    }
    refresh();
    m_shell.m_viewport->rebuildRuntime();
    m_shell.m_inspector->refresh();
    m_shell.setStatus("Created " + descriptor.displayName + " '" + nodeName + "' under " + parentId);
}

}  // namespace morrow::editor
