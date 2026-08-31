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

std::map<std::string, std::string> effectiveProperties(const SceneNodeRecord& node) {
    std::map<std::string, std::string> properties = node.properties;
    properties.emplace("position", "Vector3(0.0, 0.0, 0.0)");
    properties.emplace("rotation", "Vector3(0.0, 0.0, 0.0)");
    properties.emplace("scale", "Vector3(1.0, 1.0, 1.0)");
    properties.emplace("size", "Vector2(100.0, 40.0)");
    properties.emplace("visible", "true");
    if (node.type != "SceneNode") {
        properties.emplace("mesh", "Generated UI Quad");
        properties.emplace("renderer_enabled", "true");
        properties.emplace("material", "Default UI Material");
        properties.emplace("material_color", "Color(1.0, 1.0, 1.0, 1.0)");
        properties.emplace("blend_enabled", "true");
        properties.emplace("double_sided", "false");
    }
    if (node.type == "MRImage")
        properties.emplace("texture_asset", "");
    return properties;
}

std::string inspectorComponent(const std::string& name) {
    if (name == "position" || name == "rotation" || name == "scale" || name == "size")
        return "Transform";
    if (name == "mesh")
        return "Mesh Filter";
    if (name == "renderer_enabled" || name == "material" || name == "material_color" || name == "blend_enabled" || name == "double_sided")
        return "Mesh Renderer";
    return "Properties";
}

std::string inspectorDisplayName(const std::string& name) {
    static const std::map<std::string, std::string> displayNames = {
        {"position", "Position"},
        {"rotation", "Rotation"},
        {"scale", "Scale"},
        {"size", "Size"},
        {"mesh", "Mesh"},
        {"renderer_enabled", "Enabled"},
        {"material", "Material"},
        {"material_color", "Color"},
        {"blend_enabled", "Blend"},
        {"double_sided", "Double Sided"},
        {"visible", "Visible"},
        {"display_layer", "Display Layer"},
        {"texture_asset", "Source Image"},
    };
    const auto iterator = displayNames.find(name);
    if (iterator != displayNames.end())
        return iterator->second;

    std::string result;
    bool capitalize = true;
    for (const char character : name) {
        if (character == '_') {
            result.push_back(' ');
            capitalize = true;
        } else {
            result.push_back(capitalize ? static_cast<char>(std::toupper(static_cast<unsigned char>(character))) : character);
            capitalize = false;
        }
    }
    return result;
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
    float z = 0.0f;
    return nodeWorldRect(m_selection.nodeIds.front(), x, y, z, width, height);
}

bool SceneEditorModel::selectedLocalRect(float& x, float& y, float& z, float& width, float& height) const {
    if (m_selection.nodeIds.empty())
        return false;
    const auto* node = m_document.findNode(m_selection.nodeIds.front());
    if (!node)
        return false;
    std::vector<float> position;
    std::vector<float> size;
    const auto positionIt = node->properties.find("position");
    const auto sizeIt = node->properties.find("size");
    if (positionIt == node->properties.end() || sizeIt == node->properties.end() || !parseComponents(positionIt->second, "Vector3", position) ||
        !parseComponents(sizeIt->second, "Vector2", size) || position.size() != 3 || size.size() != 2)
        return false;
    x = position[0];
    y = position[1];
    z = position[2];
    width = size[0];
    height = size[1];
    return true;
}

bool SceneEditorModel::nodeWorldRect(const std::string& nodeId, float& x, float& y, float& z, float& width, float& height) const {
    const auto* node = m_document.findNode(nodeId);
    if (!node)
        return false;
    std::vector<float> position;
    std::vector<float> size;
    const auto positionIt = node->properties.find("position");
    const auto sizeIt = node->properties.find("size");
    if (positionIt == node->properties.end() || sizeIt == node->properties.end() || !parseComponents(positionIt->second, "Vector3", position) ||
        !parseComponents(sizeIt->second, "Vector2", size) || position.size() != 3 || size.size() != 2) {
        return false;
    }
    x = position[0];
    y = position[1];
    z = position[2];
    width = size[0];
    height = size[1];
    for (auto* parent = node->parentId.empty() ? nullptr : m_document.findNode(node->parentId); parent;
         parent = parent->parentId.empty() ? nullptr : m_document.findNode(parent->parentId)) {
        const auto parentPosition = parent->properties.find("position");
        std::vector<float> parentValues;
        if (parentPosition != parent->properties.end() && parseComponents(parentPosition->second, "Vector3", parentValues) && parentValues.size() == 3) {
            x += parentValues[0];
            y += parentValues[1];
            z += parentValues[2];
        }
    }
    return true;
}

std::vector<InspectorProperty> SceneEditorModel::inspectSelected() const {
    std::vector<InspectorProperty> result;
    if (m_selection.nodeIds.empty())
        return result;
    const auto node = m_document.findNode(m_selection.nodeIds.front());
    if (!node)
        return result;
    const auto properties = effectiveProperties(*node);
    result.reserve(properties.size());
    for (const auto& [name, value] : properties) {
        std::string type = "string";
        if (value == "true" || value == "false")
            type = "bool";
        else if (value.find("Vector2(") == 0)
            type = "Vector2";
        else if (value.find("Vector3(") == 0)
            type = "Vector3";
        else if (value.find("Color(") == 0)
            type = "Color";
        else if (name == "texture_asset")
            type = "TextureAsset";
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
            const auto selectedProperties = effectiveProperties(*selected);
            const auto selectedProperty = selectedProperties.find(name);
            if (selectedProperty == selectedProperties.end() || selectedProperty->second != value) {
                mixed = true;
                break;
            }
        }
        const bool editable = name != "mesh" && name != "material";
        result.push_back({name, mixed ? "<mixed>" : value, type, editable, mixed, inspectorComponent(name), inspectorDisplayName(name)});
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
