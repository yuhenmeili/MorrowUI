#include "CommandHistory.h"

#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <utility>

#include "scene/SceneDocument.h"

namespace morrow::editor {

namespace {
std::string nextAvailableNodeId(const SceneDocument& document) {
    for (int index = 1;; ++index) {
        std::ostringstream id;
        id << "node_" << std::setw(4) << std::setfill('0') << index;
        if (!document.findNode(id.str()))
            return id.str();
    }
}

void collectSubtree(const SceneDocument& document, const std::string& rootId, std::vector<const SceneNodeRecord*>& out) {
    const auto* node = document.findNode(rootId);
    if (!node)
        return;
    out.push_back(node);
    for (const auto& child : document.nodes()) {
        if (child.parentId == rootId)
            collectSubtree(document, child.id, out);
    }
}
}  // namespace

SetNodePropertyCommand::SetNodePropertyCommand(std::string nodeId, std::string property, std::string value) :
    m_nodeId(std::move(nodeId)), m_property(std::move(property)), m_value(std::move(value)) {
}

bool SetNodePropertyCommand::execute(SceneDocument& document, std::string& error) {
    if (!m_capturedPreviousValue) {
        const auto node = document.findNode(m_nodeId);
        if (!node) {
            error = "node '" + m_nodeId + "' was not found";
            return false;
        }
        const auto property = node->properties.find(m_property);
        m_hadPreviousValue = property != node->properties.end();
        if (m_hadPreviousValue) {
            m_previousValue = property->second;
        }
        m_capturedPreviousValue = true;
    }
    return document.setNodeProperty(m_nodeId, m_property, m_value, error);
}

bool SetNodePropertyCommand::undo(SceneDocument& document, std::string& error) {
    if (!m_capturedPreviousValue) {
        error = "command has not been executed";
        return false;
    }
    if (m_hadPreviousValue) {
        return document.setNodeProperty(m_nodeId, m_property, m_previousValue, error);
    }
    return document.removeNodeProperty(m_nodeId, m_property, error);
}

bool SetNodePropertyCommand::canMergeWith(const SceneCommand& other) const {
    const auto* candidate = dynamic_cast<const SetNodePropertyCommand*>(&other);
    return candidate && candidate->m_nodeId == m_nodeId && candidate->m_property == m_property;
}

bool SetNodePropertyCommand::mergeFrom(const SceneCommand& other) {
    const auto* candidate = dynamic_cast<const SetNodePropertyCommand*>(&other);
    if (!candidate || !canMergeWith(other))
        return false;
    m_value = candidate->m_value;
    return true;
}

ReparentNodeCommand::ReparentNodeCommand(std::string nodeId, std::string parentId) : m_nodeId(std::move(nodeId)), m_parentId(std::move(parentId)) {
}

bool ReparentNodeCommand::execute(SceneDocument& document, std::string& error) {
    if (!m_capturedPreviousParent) {
        const auto node = document.findNode(m_nodeId);
        if (!node) {
            error = "node '" + m_nodeId + "' was not found";
            return false;
        }
        m_previousParentId = node->parentId;
        m_capturedPreviousParent = true;
    }
    return document.reparentNode(m_nodeId, m_parentId, error);
}

bool ReparentNodeCommand::undo(SceneDocument& document, std::string& error) {
    if (!m_capturedPreviousParent) {
        error = "command has not been executed";
        return false;
    }
    return document.reparentNode(m_nodeId, m_previousParentId, error);
}

RenameNodeCommand::RenameNodeCommand(std::string nodeId, std::string name) : m_nodeId(std::move(nodeId)), m_name(std::move(name)) {
}

bool RenameNodeCommand::execute(SceneDocument& document, std::string& error) {
    if (!m_capturedPreviousName) {
        const auto* node = document.findNode(m_nodeId);
        if (!node) {
            error = "node '" + m_nodeId + "' was not found";
            return false;
        }
        m_previousName = node->name;
        m_capturedPreviousName = true;
    }
    return document.renameNode(m_nodeId, m_name, error);
}

bool RenameNodeCommand::undo(SceneDocument& document, std::string& error) {
    if (!m_capturedPreviousName) {
        error = "command has not been executed";
        return false;
    }
    return document.renameNode(m_nodeId, m_previousName, error);
}

SetNodeRectCommand::SetNodeRectCommand(std::string nodeId, std::string position, std::string size) :
    m_nodeId(std::move(nodeId)), m_position(std::move(position)), m_size(std::move(size)) {
}

bool SetNodeRectCommand::execute(SceneDocument& document, std::string& error) {
    if (!m_capturedPrevious) {
        const auto* node = document.findNode(m_nodeId);
        if (!node) {
            error = "node '" + m_nodeId + "' was not found";
            return false;
        }
        const auto position = node->properties.find("position");
        const auto size = node->properties.find("size");
        if (position == node->properties.end() || size == node->properties.end()) {
            error = "node requires position and size properties";
            return false;
        }
        m_previousPosition = position->second;
        m_previousSize = size->second;
        m_capturedPrevious = true;
    }
    if (!document.setNodeProperty(m_nodeId, "position", m_position, error))
        return false;
    return document.setNodeProperty(m_nodeId, "size", m_size, error);
}

bool SetNodeRectCommand::undo(SceneDocument& document, std::string& error) {
    if (!m_capturedPrevious) {
        error = "command has not been executed";
        return false;
    }
    if (!document.setNodeProperty(m_nodeId, "position", m_previousPosition, error))
        return false;
    return document.setNodeProperty(m_nodeId, "size", m_previousSize, error);
}

bool SetNodeRectCommand::canMergeWith(const SceneCommand& other) const {
    const auto* command = dynamic_cast<const SetNodeRectCommand*>(&other);
    return command && command->m_nodeId == m_nodeId;
}

bool SetNodeRectCommand::mergeFrom(const SceneCommand& other) {
    const auto* command = dynamic_cast<const SetNodeRectCommand*>(&other);
    if (!command || command->m_nodeId != m_nodeId)
        return false;
    m_position = command->m_position;
    m_size = command->m_size;
    return true;
}

namespace {
std::string offsetVector3Value(const std::string& value, float dx, float dy) {
    if (value.empty() || value.back() != ')' || value.rfind("Vector3(", 0) != 0)
        return {};
    const std::string body = value.substr(8, value.size() - 9);
    std::vector<float> components;
    size_t start = 0;
    while (start <= body.size()) {
        const auto separator = body.find(',', start);
        const auto part = body.substr(start, separator == std::string::npos ? std::string::npos : separator - start);
        try {
            components.push_back(std::stof(part));
        } catch (...) {
            return {};
        }
        if (separator == std::string::npos)
            break;
        start = separator + 1;
    }
    if (components.size() != 3)
        return {};
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "Vector3(%.3f, %.3f, %.3f)", components[0] + dx, components[1] + dy, components[2]);
    return buffer;
}
}  // namespace

MoveSelectionCommand::MoveSelectionCommand(std::vector<std::string> nodeIds, float dx, float dy) : m_nodeIds(std::move(nodeIds)), m_dx(dx), m_dy(dy) {
}

bool MoveSelectionCommand::execute(SceneDocument& document, std::string& error) {
    if (!m_capturedPrevious) {
        for (const auto& nodeId : m_nodeIds) {
            const auto node = document.findNode(nodeId);
            if (!node) {
                error = "node '" + nodeId + "' was not found";
                return false;
            }
            const auto position = node->properties.find("position");
            m_previousPositions.push_back(position == node->properties.end() ? std::string{} : position->second);
        }
        m_capturedPrevious = true;
    }
    for (size_t index = 0; index < m_nodeIds.size(); ++index) {
        const std::string offset = offsetVector3Value(m_previousPositions[index], m_dx, m_dy);
        if (offset.empty()) {
            error = "node '" + m_nodeIds[index] + "' requires a Vector3 position property";
            return false;
        }
        if (!document.setNodeProperty(m_nodeIds[index], "position", offset, error))
            return false;
    }
    return true;
}

bool MoveSelectionCommand::undo(SceneDocument& document, std::string& error) {
    if (!m_capturedPrevious) {
        error = "command has not been executed";
        return false;
    }
    for (size_t index = 0; index < m_nodeIds.size(); ++index) {
        if (m_previousPositions[index].empty()) {
            if (!document.removeNodeProperty(m_nodeIds[index], "position", error))
                return false;
        } else if (!document.setNodeProperty(m_nodeIds[index], "position", m_previousPositions[index], error)) {
            return false;
        }
    }
    return true;
}

bool MoveSelectionCommand::canMergeWith(const SceneCommand& other) const {
    const auto* candidate = dynamic_cast<const MoveSelectionCommand*>(&other);
    return candidate && candidate->m_nodeIds == m_nodeIds;
}

bool MoveSelectionCommand::mergeFrom(const SceneCommand& other) {
    const auto* candidate = dynamic_cast<const MoveSelectionCommand*>(&other);
    if (!candidate || !canMergeWith(other))
        return false;
    m_dx += candidate->m_dx;
    m_dy += candidate->m_dy;
    return true;
}

AddNodeCommand::AddNodeCommand(SceneNodeRecord node) : m_node(std::move(node)) {
}

bool AddNodeCommand::execute(SceneDocument& document, std::string& error) {
    return document.addNode(m_node, error);
}

bool AddNodeCommand::undo(SceneDocument& document, std::string& error) {
    std::vector<SceneNodeRecord> removed;
    return document.removeNodeSubtree(m_node.id, removed, error);
}

DeleteNodeCommand::DeleteNodeCommand(std::string nodeId) : m_nodeId(std::move(nodeId)) {
}

bool DeleteNodeCommand::execute(SceneDocument& document, std::string& error) {
    if (!m_captured) {
        if (!document.removeNodeSubtree(m_nodeId, m_removed, error))
            return false;
        m_captured = true;
        return true;
    }
    std::vector<SceneNodeRecord> ignored;
    return document.removeNodeSubtree(m_nodeId, ignored, error);
}

bool DeleteNodeCommand::undo(SceneDocument& document, std::string& error) {
    for (const auto& node : m_removed) {
        if (!document.addNode(node, error))
            return false;
    }
    return true;
}

DuplicateNodeCommand::DuplicateNodeCommand(std::string sourceId, std::string duplicateId) : m_sourceId(std::move(sourceId)), m_duplicateId(std::move(duplicateId)) {
}

bool DuplicateNodeCommand::execute(SceneDocument& document, std::string& error) {
    if (m_duplicates.empty()) {
        // 深度优先收集整棵源子树，父节点先于子节点出现，保证父 id 重映射先就绪
        std::vector<const SceneNodeRecord*> subtree;
        collectSubtree(document, m_sourceId, subtree);
        if (subtree.empty()) {
            error = "node '" + m_sourceId + "' was not found";
            return false;
        }
        std::map<std::string, std::string> idMap{{m_sourceId, m_duplicateId}};
        for (const auto* node : subtree) {
            SceneNodeRecord copy = *node;
            const auto mapped = idMap.find(copy.id);
            copy.id = mapped != idMap.end() ? mapped->second : nextAvailableNodeId(document);
            if (!copy.parentId.empty()) {
                const auto mappedParent = idMap.find(copy.parentId);
                if (mappedParent != idMap.end())
                    copy.parentId = mappedParent->second;
            }
            if (node->id == m_sourceId)
                copy.name += " Copy";
            idMap[node->id] = copy.id;
            m_duplicates.push_back(std::move(copy));
        }
    }
    for (const auto& node : m_duplicates) {
        if (!document.addNode(node, error))
            return false;
    }
    return true;
}

bool DuplicateNodeCommand::undo(SceneDocument& document, std::string& error) {
    std::vector<SceneNodeRecord> removed;
    return document.removeNodeSubtree(m_duplicateId, removed, error);
}

AddNodesCommand::AddNodesCommand(std::vector<SceneNodeRecord> nodes) : m_nodes(std::move(nodes)) {
}

bool AddNodesCommand::execute(SceneDocument& document, std::string& error) {
    if (!m_capturedRoots) {
        // 批量记录中被其他记录引用为 parent 的不是根；根在 undo 时整棵移除
        for (const auto& node : m_nodes) {
            const bool referencedAsParent =
                std::any_of(m_nodes.begin(), m_nodes.end(), [&node](const SceneNodeRecord& candidate) { return candidate.parentId == node.id; });
            if (!referencedAsParent)
                m_rootIds.push_back(node.id);
        }
        m_capturedRoots = true;
    }
    for (const auto& node : m_nodes) {
        if (!document.addNode(node, error))
            return false;
    }
    return true;
}

bool AddNodesCommand::undo(SceneDocument& document, std::string& error) {
    for (const auto& rootId : m_rootIds) {
        std::vector<SceneNodeRecord> removed;
        if (!document.removeNodeSubtree(rootId, removed, error))
            return false;
    }
    return true;
}

MacroCommand::MacroCommand(std::vector<std::unique_ptr<SceneCommand>> commands) : m_commands(std::move(commands)) {
}

bool MacroCommand::execute(SceneDocument& document, std::string& error) {
    for (const auto& command : m_commands) {
        if (!command->execute(document, error))
            return false;
    }
    return true;
}

bool MacroCommand::undo(SceneDocument& document, std::string& error) {
    for (auto command = m_commands.rbegin(); command != m_commands.rend(); ++command) {
        if (!(*command)->undo(document, error))
            return false;
    }
    return true;
}

bool CommandHistory::execute(std::unique_ptr<SceneCommand> command, SceneDocument& document, std::string& error) {
    if (!command) {
        error = "command cannot be null";
        return false;
    }
    if (!command->execute(document, error)) {
        return false;
    }
    m_undoStack.push_back(std::move(command));
    m_redoStack.clear();
    return true;
}

bool CommandHistory::executeOrMerge(std::unique_ptr<SceneCommand> command, SceneDocument& document, std::string& error) {
    if (!command) {
        error = "command cannot be null";
        return false;
    }
    if (!command->execute(document, error))
        return false;
    if (!m_undoStack.empty() && m_undoStack.back()->canMergeWith(*command)) {
        return m_undoStack.back()->mergeFrom(*command);
    }
    m_undoStack.push_back(std::move(command));
    m_redoStack.clear();
    return true;
}

bool CommandHistory::undo(SceneDocument& document, std::string& error) {
    if (m_undoStack.empty()) {
        error = "nothing to undo";
        return false;
    }
    auto command = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    if (!command->undo(document, error)) {
        m_undoStack.push_back(std::move(command));
        return false;
    }
    m_redoStack.push_back(std::move(command));
    return true;
}

bool CommandHistory::redo(SceneDocument& document, std::string& error) {
    if (m_redoStack.empty()) {
        error = "nothing to redo";
        return false;
    }
    auto command = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    if (!command->execute(document, error)) {
        m_redoStack.push_back(std::move(command));
        return false;
    }
    m_undoStack.push_back(std::move(command));
    return true;
}

bool CommandHistory::canUndo() const {
    return !m_undoStack.empty();
}

bool CommandHistory::canRedo() const {
    return !m_redoStack.empty();
}

void CommandHistory::clear() {
    m_undoStack.clear();
    m_redoStack.clear();
}

}  // namespace morrow::editor
