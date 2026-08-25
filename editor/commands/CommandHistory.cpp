#include "CommandHistory.h"

#include <utility>

#include "scene/SceneDocument.h"

namespace morrow::editor {

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
        const auto source = document.findNode(m_sourceId);
        if (!source) {
            error = "node '" + m_sourceId + "' was not found";
            return false;
        }
        SceneNodeRecord copy = *source;
        copy.id = m_duplicateId;
        copy.name += " Copy";
        m_duplicates.push_back(std::move(copy));
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
