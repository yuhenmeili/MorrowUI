#ifndef MORROW_EDITOR_SESSION_H
#define MORROW_EDITOR_SESSION_H

#include <filesystem>
#include <string>
#include <vector>

#include "commands/CommandHistory.h"
#include "scene/SceneEditorModel.h"

namespace morrow::editor {

class EditorSession {
public:
    explicit EditorSession(std::filesystem::path scenePath);

    bool load(std::string& error);

    bool save(std::string& error);

    SceneDocument& document();

    const SceneDocument& document() const;

    SceneEditorModel& model();

    const SceneEditorModel& model() const;

    bool selectNode(const std::string& nodeId, bool additive, std::string& error);

    bool selectAt(float x, float y, std::string& error);

    bool setProperty(const std::string& nodeId, const std::string& property, std::string value, bool continuous, std::string& error);

    bool addNode(SceneNodeRecord node, std::string& error);

    bool deleteNode(const std::string& nodeId, std::string& error);

    bool duplicateNode(const std::string& nodeId, const std::string& duplicateId, std::string& error);

    bool reparentNode(const std::string& nodeId, const std::string& parentId, std::string& error);

    bool moveGizmo(const std::string& nodeId, float x, float y, float z, bool continuous, std::string& error);

    bool resizeGizmo(const std::string& nodeId, float width, float height, bool continuous, std::string& error);

    bool undo(std::string& error);

    bool redo(std::string& error);

    bool isDirty() const;

private:
    std::filesystem::path m_scenePath;
    SceneDocument m_document;
    SceneEditorModel m_model;
    CommandHistory m_history;
    bool m_dirty = false;
};

}  // namespace morrow::editor

#endif  // MORROW_EDITOR_SESSION_H
