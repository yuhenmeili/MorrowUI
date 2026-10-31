#ifndef MORROW_EDITOR_SCENE_EDITOR_MODEL_H
#define MORROW_EDITOR_SCENE_EDITOR_MODEL_H

#include <string>
#include <vector>

#include "SceneDocument.h"

namespace morrow::editor {
struct SceneTreeItem {
    std::string id;
    std::string parentId;
    std::string name;
    std::string type;
    int depth = 0;
    bool locked = false;
};

struct InspectorProperty {
    std::string name;
    std::string value;
    std::string type;
    bool editable = true;
    bool mixed = false;
    std::string component;
    std::string displayName;
    /// 与 schema 默认不同时的默认值（非空 → Inspector 显示重置按钮）。
    std::string defaultValue;
    /// number 属性的取值范围（渲染为滑块）。
    bool hasRange = false;
    float minValue = 0.0f;
    float maxValue = 0.0f;
    float stepValue = 1.0f;
    /// enum 属性的取值表（渲染为下拉）。
    std::vector<std::pair<std::string, std::string>> enumOptions;
};

struct SelectionState {
    std::vector<std::string> nodeIds;
    float gizmoSnap = 1.0f;
    bool gizmoActive = false;
};

class SceneEditorModel {
public:
    explicit SceneEditorModel(SceneDocument& document);

    std::vector<SceneTreeItem> buildSceneTree() const;

    bool selectNode(const std::string& nodeId, bool additive, std::string& error);

    void clearSelection();

    const SelectionState& selection() const;

    bool selectedRect(float& x, float& y, float& width, float& height) const;

    bool selectedLocalRect(float& x, float& y, float& z, float& width, float& height) const;

    bool nodeWorldRect(const std::string& nodeId, float& x, float& y, float& z, float& width, float& height) const;

    std::vector<InspectorProperty> inspectSelected() const;

    bool applyGizmoPosition(const std::string& nodeId, float x, float y, float z, std::string& error);

    bool applyGizmoSize(const std::string& nodeId, float width, float height, std::string& error);

private:
    void appendTree(const SceneNodeRecord& node, int depth, std::vector<SceneTreeItem>& result) const;

    SceneDocument& m_document;
    SelectionState m_selection;
};
} // namespace morrow::editor

#endif  // MORROW_EDITOR_SCENE_EDITOR_MODEL_H
