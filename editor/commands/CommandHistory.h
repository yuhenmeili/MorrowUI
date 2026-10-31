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

    virtual bool canMergeWith(const SceneCommand& other) const {
        return false;
    }

    virtual bool mergeFrom(const SceneCommand& other) {
        return false;
    }
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
    SetNodeRectCommand(std::string nodeId, std::string position, std::string size);

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

/// 整组选中节点平移：多选拖拽与方向键微调共用，单条撤销记录，可按节点组连续合并。
class MoveSelectionCommand final : public SceneCommand {
public:
    MoveSelectionCommand(std::vector<std::string> nodeIds, float dx, float dy);

    bool execute(SceneDocument& document, std::string& error) override;

    bool undo(SceneDocument& document, std::string& error) override;

    bool canMergeWith(const SceneCommand& other) const override;

    bool mergeFrom(const SceneCommand& other) override;

private:
    std::vector<std::string> m_nodeIds;
    float m_dx = 0.0f;
    float m_dy = 0.0f;
    std::vector<std::string> m_previousPositions;
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

/// 批量添加整棵（或多棵）子树：粘贴与多选副本共用，撤销时整体移除。
class AddNodesCommand final : public SceneCommand {
public:
    explicit AddNodesCommand(std::vector<SceneNodeRecord> nodes);

    bool execute(SceneDocument& document, std::string& error) override;

    bool undo(SceneDocument& document, std::string& error) override;

private:
    std::vector<SceneNodeRecord> m_nodes;
    std::vector<std::string> m_rootIds;
    bool m_capturedRoots = false;
};

/// 组合命令：execute 顺序执行全部子命令，undo 逆序回滚，作为单条历史记录。
class MacroCommand final : public SceneCommand {
public:
    explicit MacroCommand(std::vector<std::unique_ptr<SceneCommand>> commands);

    bool execute(SceneDocument& document, std::string& error) override;

    bool undo(SceneDocument& document, std::string& error) override;

private:
    std::vector<std::unique_ptr<SceneCommand>> m_commands;
};

class CommandHistory {
public:
    bool execute(std::unique_ptr<SceneCommand> command, SceneDocument& document, std::string& error);

    bool executeOrMerge(std::unique_ptr<SceneCommand> command, SceneDocument& document, std::string& error);

    bool undo(SceneDocument& document, std::string& error);

    bool redo(SceneDocument& document, std::string& error);

    bool canUndo() const;

    bool canRedo() const;

    void clear();

private:
    std::vector<std::unique_ptr<SceneCommand>> m_undoStack;
    std::vector<std::unique_ptr<SceneCommand>> m_redoStack;
};
} // namespace morrow::editor

#endif  // MORROW_EDITOR_COMMAND_HISTORY_H