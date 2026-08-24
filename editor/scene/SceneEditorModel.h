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
};

struct InspectorProperty {
    std::string name;
    std::string value;
    std::string type;
    bool editable = true;
    bool mixed = false;
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

    bool selectedLocalRect(
        float& x, float& y, float& z,
        float& width, float& height) const;

    bool nodeWorldRect(
        const std::string& nodeId,
        float& x, float& y, float& z,
        float& width, float& height) const;

    std::vector<InspectorProperty> inspectSelected() const;

    bool applyGizmoPosition(const std::string& nodeId, float x, float y, float z, std::string& error);

    bool applyGizmoSize(const std::string& nodeId, float width, float height, std::string& error);

private:
    void appendTree(const SceneNodeRecord& node, int depth, std::vector<SceneTreeItem>& result) const;

    SceneDocument& m_document;
    SelectionState m_selection;
};

}  // namespace morrow::editor

#endif  // MORROW_EDITOR_SCENE_EDITOR_MODEL_H
