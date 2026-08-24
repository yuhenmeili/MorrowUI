#ifndef MORROW_EDITOR_COMMAND_HISTORY_H
#define MORROW_EDITOR_COMMAND_HISTORY_H

#include <memory>
#include <string>
#include <vector>

#include "scene/SceneDocument.h"

namespace morrow::editor {

class SceneCommand {
public:
    virtual ~SceneCommand() = default;

    virtual bool execute(SceneDocument& document, std::string& error) = 0;

    virtual bool undo(SceneDocument& document, std::string& error) = 0;

    virtual bool canMergeWith(const SceneCommand& other) const { return false; }

    virtual bool mergeFrom(const SceneCommand& other) { return false; }
};

class SetNodePropertyCommand final : public SceneCommand {
public:
    SetNodePropertyCommand(std::string nodeId, std::string property, std::string value);

    bool execute(SceneDocument& document, std::string& error) override;

    bool undo(SceneDocument& document, std::string& error) override;

    bool canMergeWith(const SceneCommand& other) const override;

    bool mergeFrom(const SceneCommand& other) override;

private:
    std::string m_nodeId;
    std::string m_property;
    std::string m_value;
    std::string m_previousValue;
    bool m_hadPreviousValue = false;
    bool m_capturedPreviousValue = false;
};

class ReparentNodeCommand final : public SceneCommand {
public:
    ReparentNodeCommand(std::string nodeId, std::string parentId);

    bool execute(SceneDocument& document, std::string& error) override;

    bool undo(SceneDocument& document, std::string& error) override;

private:
    std::string m_nodeId;
    std::string m_parentId;
    std::string m_previousParentId;
    bool m_capturedPreviousParent = false;
};

class RenameNodeCommand final : public SceneCommand {
public:
    RenameNodeCommand(std::string nodeId, std::string name);

    bool execute(SceneDocument& document, std::string& error) override;

    bool undo(SceneDocument& document, std::string& error) override;

private:
    std::string m_nodeId;
    std::string m_name;
    std::string m_previousName;
    bool m_capturedPreviousName = false;
};

class SetNodeRectCommand final : public SceneCommand {
public:
    SetNodeRectCommand(
        std::string nodeId,
        std::string position,
        std::string size);

    bool execute(SceneDocument& document, std::string& error) override;

    bool undo(SceneDocument& document, std::string& error) override;

    bool canMergeWith(const SceneCommand& other) const override;

    bool mergeFrom(const SceneCommand& other) override;

private:
    std::string m_nodeId;
    std::string m_position;
    std::string m_size;
    std::string m_previousPosition;
    std::string m_previousSize;
    bool m_capturedPrevious = false;
};

class AddNodeCommand final : public SceneCommand {
public:
    explicit AddNodeCommand(SceneNodeRecord node);
    bool execute(SceneDocument& document, std::string& error) override;
    bool undo(SceneDocument& document, std::string& error) override;
private:
    SceneNodeRecord m_node;
};

class DeleteNodeCommand final : public SceneCommand {
public:
    explicit DeleteNodeCommand(std::string nodeId);
    bool execute(SceneDocument& document, std::string& error) override;
    bool undo(SceneDocument& document, std::string& error) override;
private:
    std::string m_nodeId;
    std::vector<SceneNodeRecord> m_removed;
    bool m_captured = false;
};

class DuplicateNodeCommand final : public SceneCommand {
public:
    DuplicateNodeCommand(std::string sourceId, std::string duplicateId);
    bool execute(SceneDocument& document, std::string& error) override;
    bool undo(SceneDocument& document, std::string& error) override;
private:
    std::string m_sourceId;
    std::string m_duplicateId;
    std::vector<SceneNodeRecord> m_duplicates;
};

class CommandHistory {
public:
    bool execute(std::unique_ptr<SceneCommand> command, SceneDocument& document, std::string& error);

    bool executeOrMerge(std::unique_ptr<SceneCommand> command,
                        SceneDocument& document,
                        std::string& error);

    bool undo(SceneDocument& document, std::string& error);

    bool redo(SceneDocument& document, std::string& error);

    bool canUndo() const;

    bool canRedo() const;

    void clear();

private:
    std::vector<std::unique_ptr<SceneCommand>> m_undoStack;
    std::vector<std::unique_ptr<SceneCommand>> m_redoStack;
};

}  // namespace morrow::editor

#endif  // MORROW_EDITOR_COMMAND_HISTORY_H
