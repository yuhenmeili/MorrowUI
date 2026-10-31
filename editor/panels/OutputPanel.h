#ifndef MORROW_EDITOR_OUTPUT_PANEL_H
#define MORROW_EDITOR_OUTPUT_PANEL_H

#include <memory>
#include <string>
#include <vector>
#include "panels/EditorPanel.h"

namespace morrow {
class MRTextEdit;
class MRLineEdit;
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

    /// 清空输出（Clear 按钮）。
    void clear();

    /// 设置子串过滤词（空 = 不过滤），立即刷新。
    void setFilter(const std::string& filter);

    const std::string& filter() const {
        return m_filter;
    }

    std::shared_ptr<UIWidget>& panel = m_root;
    std::shared_ptr<MRTextEdit> log;
    /// 过滤输入框（EditorShell 创建并回填）。
    std::shared_ptr<MRLineEdit> filterEdit;
    std::vector<std::string> lines;

private:
    friend class EditorShell;
    EditorShell& m_shell;
    std::string m_filter;
};
} // namespace morrow::editor

#endif
