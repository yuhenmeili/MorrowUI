#include "SceneEditorModel.h"

#include <algorithm>
#include <cctype>

namespace morrow::editor {

SceneEditorModel::SceneEditorModel(SceneDocument& document) : m_document(document) {
}

void SceneEditorModel::appendTree(const SceneNodeRecord& node, int depth, std::vector<SceneTreeItem>& result) const {
    result.push_back({node.id, node.parentId, node.name, node.type, depth});
    for (const auto& child : m_document.nodes()) {
        if (child.parentId == node.id)
            appendTree(child, depth + 1, result);
    }
}

std::vector<SceneTreeItem> SceneEditorModel::buildSceneTree() const {
    std::vector<SceneTreeItem> result;
    for (const auto& node : m_document.nodes()) {
        if (node.parentId.empty())
            appendTree(node, 0, result);
    }
    return result;
}

bool SceneEditorModel::selectNode(const std::string& nodeId, bool additive, std::string& error) {
    if (!m_document.findNode(nodeId)) {
        error = "node '" + nodeId + "' was not found";
        return false;
    }
    if (!additive)
        m_selection.nodeIds.clear();
    if (std::find(m_selection.nodeIds.begin(), m_selection.nodeIds.end(), nodeId) == m_selection.nodeIds.end()) {
        m_selection.nodeIds.push_back(nodeId);
    }
    return true;
}

const SelectionState& SceneEditorModel::selection() const {
    return m_selection;
}

std::vector<InspectorProperty> SceneEditorModel::inspectSelected() const {
    std::vector<InspectorProperty> result;
    if (m_selection.nodeIds.empty())
        return result;
    const auto node = m_document.findNode(m_selection.nodeIds.front());
    if (!node)
        return result;
    result.reserve(node->properties.size());
    for (const auto& [name, value] : node->properties) {
        std::string type = "string";
        if (value == "true" || value == "false")
            type = "bool";
        else if (value.find("Vector2(") == 0)
            type = "Vector2";
        else if (value.find("Vector3(") == 0)
            type = "Vector3";
        else if (value.find("Color(") == 0)
            type = "Color";
        else if (!value.empty() && (std::isdigit(static_cast<unsigned char>(value.front())) || value.front() == '-')) {
            type = "number";
        }
        result.push_back({name, value, type, true});
    }
    return result;
}

bool SceneEditorModel::applyGizmoPosition(const std::string& nodeId, float x, float y, float z, std::string& error) {
    const auto value = "Vector3(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
    return m_document.setNodeProperty(nodeId, "position", value, error);
}

bool SceneEditorModel::applyGizmoSize(const std::string& nodeId, float width, float height, std::string& error) {
    const auto value = "Vector2(" + std::to_string(width) + ", " + std::to_string(height) + ")";
    return m_document.setNodeProperty(nodeId, "size", value, error);
}

}  // namespace morrow::editor
