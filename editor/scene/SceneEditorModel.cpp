#include "SceneEditorModel.h"

#include <algorithm>
#include <cctype>

namespace morrow::editor {

namespace {
bool parseComponents(const std::string& value, const std::string& type, std::vector<float>& result) {
    if (value.rfind(type + "(", 0) != 0 || value.back() != ')')
        return false;
    size_t start = type.size() + 1;
    while (start < value.size() - 1) {
        const auto separator = value.find(',', start);
        try {
            result.push_back(std::stof(value.substr(start, separator == std::string::npos ? value.size() - 1 - start : separator - start)));
        } catch (...) {
            return false;
        }
        if (separator == std::string::npos)
            break;
        start = separator + 1;
    }
    return true;
}
}  // namespace

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

void SceneEditorModel::clearSelection() {
    m_selection.nodeIds.clear();
}

const SelectionState& SceneEditorModel::selection() const {
    return m_selection;
}

bool SceneEditorModel::selectedRect(float& x, float& y, float& width, float& height) const {
    if (m_selection.nodeIds.empty())
        return false;
    const auto node = m_document.findNode(m_selection.nodeIds.front());
    if (!node)
        return false;
    std::vector<float> position;
    std::vector<float> size;
    const auto positionIt = node->properties.find("position");
    const auto sizeIt = node->properties.find("size");
    if (positionIt == node->properties.end() || sizeIt == node->properties.end() ||
        !parseComponents(positionIt->second, "Vector3", position) || !parseComponents(sizeIt->second, "Vector2", size) ||
        position.size() != 3 || size.size() != 2)
        return false;
    x = position[0];
    y = position[1];
    width = size[0];
    height = size[1];
    return true;
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
        bool mixed = false;
        for (size_t selectionIndex = 1; selectionIndex < m_selection.nodeIds.size(); ++selectionIndex) {
            const auto selected = m_document.findNode(m_selection.nodeIds[selectionIndex]);
            if (!selected) {
                mixed = true;
                break;
            }
            const auto selectedProperty = selected->properties.find(name);
            if (selectedProperty == selected->properties.end() || selectedProperty->second != value) {
                mixed = true;
                break;
            }
        }
        result.push_back({name, mixed ? "<mixed>" : value, type, true, mixed});
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
