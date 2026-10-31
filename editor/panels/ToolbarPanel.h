#ifndef MORROW_EDITOR_TOOLBAR_PANEL_H
#define MORROW_EDITOR_TOOLBAR_PANEL_H

#include <memory>
#include "panels/EditorPanel.h"

namespace morrow {
class MRButton;
class UIWidget;
}

namespace morrow::editor {
class EditorShell;

class ToolbarPanel : public EditorPanel {
public:
    explicit ToolbarPanel(EditorShell& shell);

    void build();

    /// 视口吸附开关/步长变化后刷新按钮文案。
    void refreshSnapButtons();

private:
    EditorShell& m_shell;

public:
    std::shared_ptr<UIWidget>& panel = m_root;
    std::shared_ptr<morrow::MRButton> snapButton;
    std::shared_ptr<morrow::MRButton> stepButton;
};
}  // namespace morrow::editor

#endif
