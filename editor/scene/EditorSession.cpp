#include "EditorSession.h"

#include <algorithm>
#include <cstdio>
#include <memory>

namespace {

bool parseVector(const std::string& value, const std::string& type, std::vector<float>& components) {
    if (value.compare(0, type.size() + 1, type + "(") != 0 || value.empty() || value.back() != ')') {
        return false;
    }
    std::string body = value.substr(type.size() + 1, value.size() - type.size() - 2);
    size_t start = 0;
    while (start <= body.size()) {
        const auto separator = body.find(',', start);
        const auto part = body.substr(start, separator == std::string::npos ? std::string::npos : separator - start);
        try {
            components.push_back(std::stof(part));
        } catch (...) {
            return false;
        }
        if (separator == std::string::npos)
            break;
        start = separator + 1;
    }
    return true;
}

std::string vector2(float x, float y) {
    char buffer[96];
    std::snprintf(buffer, sizeof(buffer), "Vector2(%.3f, %.3f)", x, y);
    return buffer;
}

std::string vector3(float x, float y, float z) {
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "Vector3(%.3f, %.3f, %.3f)", x, y, z);
    return buffer;
}

}  // namespace

namespace morrow::editor {

EditorSession::EditorSession(std::filesystem::path scenePath) : m_scenePath(std::move(scenePath)), m_model(m_document) {
}

bool EditorSession::load(std::string& error) {
    if (!SceneDocument::loadFromFile(m_scenePath, m_document, error)) {
        return false;
    }
    m_history.clear();
    m_dirty = false;
    return true;
}

bool EditorSession::save(std::string& error) {
    if (!m_document.saveToFile(m_scenePath, error))
        return false;
    m_dirty = false;
    return true;
}

SceneDocument& EditorSession::document() {
    return m_document;
}

const SceneDocument& EditorSession::document() const {
    return m_document;
}

SceneEditorModel& EditorSession::model() {
    return m_model;
}

const SceneEditorModel& EditorSession::model() const {
    return m_model;
}

bool EditorSession::selectNode(const std::string& nodeId, bool additive, std::string& error) {
    return m_model.selectNode(nodeId, additive, error);
}

bool EditorSession::selectAt(float x, float y, std::string& error, bool additive) {
    const auto& nodes = m_document.nodes();
    for (auto iterator = nodes.rbegin(); iterator != nodes.rend(); ++iterator) {
        float nodeX = 0.0f;
        float nodeY = 0.0f;
        float nodeZ = 0.0f;
        float nodeWidth = 0.0f;
        float nodeHeight = 0.0f;
        if (!m_model.nodeWorldRect(
                iterator->id, nodeX, nodeY, nodeZ,
                nodeWidth, nodeHeight)) {
            continue;
        }
        if (x >= nodeX && y >= nodeY &&
            x <= nodeX + nodeWidth && y <= nodeY + nodeHeight) {
            return m_model.selectNode(iterator->id, additive, error);
        }
    }
    error = "no selectable 2D node at the requested position";
    return false;
}

bool EditorSession::moveSelection(float dx, float dy, bool continuous, std::string& error) {
    bool changed = false;
    const auto selected = m_model.selection().nodeIds;
    for (const auto& nodeId : selected) {
        const auto node = m_document.findNode(nodeId);
        if (!node)
            continue;
        const auto position = node->properties.find("position");
        std::vector<float> values;
        if (position == node->properties.end() || !parseVector(position->second, "Vector3", values) || values.size() != 3)
            continue;
        if (!moveGizmo(nodeId, values[0] + dx, values[1] + dy, values[2], continuous, error))
            return false;
        changed = true;
    }
    return changed;
}

bool EditorSession::setProperty(const std::string& nodeId, const std::string& property, std::string value, bool continuous, std::string& error) {
    auto command = std::make_unique<SetNodePropertyCommand>(nodeId, property, std::move(value));
    const bool changed = continuous ? m_history.executeOrMerge(std::move(command), m_document, error) : m_history.execute(std::move(command), m_document, error);
    if (changed)
        m_dirty = true;
    return changed;
}

bool EditorSession::addNode(SceneNodeRecord node, std::string& error) {
    const bool changed = m_history.execute(std::make_unique<AddNodeCommand>(std::move(node)), m_document, error);
    if (changed)
        m_dirty = true;
    return changed;
}

bool EditorSession::deleteNode(const std::string& nodeId, std::string& error) {
    const bool changed = m_history.execute(std::make_unique<DeleteNodeCommand>(nodeId), m_document, error);
    if (changed)
        m_dirty = true;
    return changed;
}

bool EditorSession::duplicateNode(const std::string& nodeId, const std::string& duplicateId, std::string& error) {
    const bool changed = m_history.execute(std::make_unique<DuplicateNodeCommand>(nodeId, duplicateId), m_document, error);
    if (changed)
        m_dirty = true;
    return changed;
}

bool EditorSession::reparentNode(const std::string& nodeId, const std::string& parentId, std::string& error) {
    const bool changed = m_history.execute(std::make_unique<ReparentNodeCommand>(nodeId, parentId), m_document, error);
    if (changed)
        m_dirty = true;
    return changed;
}

bool EditorSession::renameNode(
    const std::string& nodeId,
    std::string name,
    std::string& error) {
    const bool changed = m_history.execute(
        std::make_unique<RenameNodeCommand>(
            nodeId, std::move(name)),
        m_document, error);
    if (changed)
        m_dirty = true;
    return changed;
}

bool EditorSession::moveGizmo(const std::string& nodeId, float x, float y, float z, bool continuous, std::string& error) {
    return setProperty(nodeId, "position", vector3(x, y, z), continuous, error);
}

bool EditorSession::resizeGizmo(const std::string& nodeId, float width, float height, bool continuous, std::string& error) {
    return setProperty(nodeId, "size", vector2(width, height), continuous, error);
}

bool EditorSession::setNodeRect(
    const std::string& nodeId,
    float x, float y, float z,
    float width, float height,
    bool continuous,
    std::string& error) {
    auto command = std::make_unique<SetNodeRectCommand>(
        nodeId, vector3(x, y, z), vector2(width, height));
    const bool changed =
        continuous
            ? m_history.executeOrMerge(
                  std::move(command), m_document, error)
            : m_history.execute(std::move(command), m_document, error);
    if (changed)
        m_dirty = true;
    return changed;
}

bool EditorSession::undo(std::string& error) {
    const bool changed = m_history.undo(m_document, error);
    if (changed)
        m_dirty = true;
    return changed;
}

bool EditorSession::redo(std::string& error) {
    const bool changed = m_history.redo(m_document, error);
    if (changed)
        m_dirty = true;
    return changed;
}

bool EditorSession::isDirty() const {
    return m_dirty;
}

}  // namespace morrow::editor
