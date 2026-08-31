#ifndef MORROW_EDITOR_SCENE_TREE_PANEL_H
#define MORROW_EDITOR_SCENE_TREE_PANEL_H

#include <memory>
#include <string>
#include <vector>

#include "core/Observable.h"
#include "ui/CreateNodeDialog.h"
#include "panels/EditorPanel.h"

namespace morrow {
class MRButton;
class MRLineEdit;
class MRPopupMenu;
class TouchEvent;
class UIWidget;
class Widget;
} // namespace morrow

namespace morrow::editor {
class EditorShell;

struct SceneTreeRow {
    std::string nodeId;
    std::shared_ptr<MRButton> button;
};

class SceneTreePanel : public EditorPanel {
public:
    explicit SceneTreePanel(EditorShell& shell);

    void refresh();

    void refreshSelectionStyles();

    bool handleDrag(const TouchEvent& event);

    void deleteSelected();

    void beginRename(const std::string& nodeId = {});

    void commitRename();

    void cancelRename();

    void showCreateDialog(const std::string& parentId = {});

    void createChild(const NodeTypeDescriptor& descriptor, const std::string& parentId);

private:
    friend class EditorShell;

    std::shared_ptr<UIWidget>& panel = m_root;
    std::shared_ptr<MRLineEdit> renameEdit;
    std::shared_ptr<MRPopupMenu> contextMenu;
    std::shared_ptr<CreateNodeDialog> createDialog;
    std::vector<EventConnection> contextConnections;
    std::vector<SceneTreeRow> rows;
    Observable<MRPopupMenu&, int, const std::wstring&>::Connection contextMenuConnection;
    Observable<CreateNodeDialog&, const NodeTypeDescriptor&, const std::string&>::Connection createConnection;
    std::string contextParentId;
    std::string renameNodeId;
    std::string pendingDragNode;
    std::string dragNode;
    std::string dropTarget;
    float dragStartX = 0.0f;
    float dragStartY = 0.0f;
    bool dragging = false;

    std::string nodeAt(float x, float y) const;

    std::string nodeForWidget(const std::shared_ptr<Widget>& widget) const;

    void renameNode(const std::string& nodeId, const std::string& name);

    EditorShell& m_shell;
};
} // namespace morrow::editor

#endif  // MORROW_EDITOR_SCENE_TREE_PANEL_H
