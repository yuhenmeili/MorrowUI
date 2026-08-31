#ifndef MORROW_EDITOR_OUTPUT_PANEL_H
#define MORROW_EDITOR_OUTPUT_PANEL_H

#include <memory>
#include <string>
#include <vector>
#include "panels/EditorPanel.h"

namespace morrow {
class MRTextEdit;
class UIWidget;
} // namespace morrow

namespace morrow::editor {
class EditorShell;

class OutputPanel : public EditorPanel {
public:
    explicit OutputPanel(EditorShell& shell);

    void refresh();

    void append(const std::string& line);

    void resize();

    std::shared_ptr<UIWidget>& panel = m_root;
    std::shared_ptr<MRTextEdit> log;
    std::vector<std::string> lines;

private:
    friend class EditorShell;
    EditorShell& m_shell;
};
} // namespace morrow::editor

#endif
