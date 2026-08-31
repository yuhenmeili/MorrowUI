#ifndef MORROW_EDITOR_TOOLBAR_PANEL_H
#define MORROW_EDITOR_TOOLBAR_PANEL_H

#include <memory>
#include "panels/EditorPanel.h"

namespace morrow {
class UIWidget;
}

namespace morrow::editor {
class EditorShell;

class ToolbarPanel : public EditorPanel {
public:
    explicit ToolbarPanel(EditorShell& shell);

    void build();

private:
    EditorShell& m_shell;

public:
    std::shared_ptr<UIWidget>& panel = m_root;
};
} // namespace morrow::editor

#endif
