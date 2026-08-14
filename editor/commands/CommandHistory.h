#ifndef MORROW_EDITOR_COMMAND_HISTORY_H
#define MORROW_EDITOR_COMMAND_HISTORY_H

#include <memory>
#include <string>
#include <vector>

namespace morrow::editor {

class SceneDocument;

class SceneCommand {
public:
    virtual ~SceneCommand() = default;

    virtual bool execute(SceneDocument& document, std::string& error) = 0;

    virtual bool undo(SceneDocument& document, std::string& error) = 0;
};

class SetNodePropertyCommand final : public SceneCommand {
public:
    SetNodePropertyCommand(std::string nodeId, std::string property, std::string value);

    bool execute(SceneDocument& document, std::string& error) override;

    bool undo(SceneDocument& document, std::string& error) override;

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

class CommandHistory {
public:
    bool execute(std::unique_ptr<SceneCommand> command, SceneDocument& document, std::string& error);

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
