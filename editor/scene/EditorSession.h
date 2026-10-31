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

    bool selectAt(float x, float y, std::string& error, bool additive = false);

    bool setProperty(const std::string& nodeId, const std::string& property, std::string value, bool continuous, std::string& error);

    bool addNode(SceneNodeRecord node, std::string& error);

    bool deleteNode(const std::string& nodeId, std::string& error);

    bool duplicateNode(const std::string& nodeId, const std::string& duplicateId, std::string& error);

    /// 多选副本（每个选中根整棵子树克隆，单条撤销记录），成功后选中新根。
    bool duplicateSelection(std::string& error);

    /// 把选中根子树（拍平为深度序列）存入会话剪贴板。
    bool copySelection(std::string& error);

    /// 复制后删除选中子树，单条撤销记录。
    bool cutSelection(std::string& error);

    /// 把剪贴板子树粘贴到当前选中节点（无选中则贴到场景根），根整体偏移(8,8)，返回新根 id。
    bool pasteClipboard(std::vector<std::string>& pastedRootIds, std::string& error);

    bool hasClipboard() const;

    bool reparentNode(const std::string& nodeId, const std::string& parentId, std::string& error);

    bool renameNode(const std::string& nodeId, std::string name, std::string& error);

    bool moveGizmo(const std::string& nodeId, float x, float y, float z, bool continuous, std::string& error);

    bool resizeGizmo(const std::string& nodeId, float width, float height, bool continuous, std::string& error);

    bool rotateGizmo(const std::string& nodeId, float zDegrees, bool continuous, std::string& error);

    bool setNodeRect(const std::string& nodeId, float x, float y, float z, float width, float height, bool continuous, std::string& error);

    bool moveSelection(float dx, float dy, bool continuous, std::string& error);

    bool undo(std::string& error);

    bool redo(std::string& error);

    bool isDirty() const;

    /// 直接文档修改（如参考线增删）之后手动置脏。
    void markDirty();

    /// 清空全部视口参考线。
    bool clearGuides(std::string& error);

private:
    std::filesystem::path m_scenePath;
    SceneDocument m_document;
    SceneEditorModel m_model;
    CommandHistory m_history;
    std::vector<SceneNodeRecord> m_clipboard;
    bool m_dirty = false;
};
} // namespace morrow::editor

#endif  // MORROW_EDITOR_SESSION_H