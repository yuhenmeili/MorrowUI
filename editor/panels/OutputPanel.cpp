#include "panels/OutputPanel.h"

#include <algorithm>
#include <codecvt>
#include <locale>

#include "EditorShell.h"
#include "base/Transform.h"
#include "elements/MRTextEdit.h"
#include "panels/BuildPanel.h"

namespace morrow::editor {
OutputPanel::OutputPanel(EditorShell& shell) : m_shell(shell) {
}

void OutputPanel::append(const std::string& line) {
    lines.push_back(line);
    while (lines.size() > 6)
        lines.erase(lines.begin());
    m_shell.m_outputLines = lines;
    refresh();
}

void OutputPanel::resize() {
    refresh();
}

void OutputPanel::refresh() {
    if (!panel || !m_shell.m_build->panel)
        return;
    std::wstring text;
    lines = m_shell.m_outputLines;
    for (size_t index = 0; index < lines.size(); ++index) {
        if (index > 0)
            text.push_back(L'\n');
        text += std::wstring(lines[index].begin(), lines[index].end());
    }
    const auto updateLog = [&text](const std::shared_ptr<MRTextEdit>& edit, const std::shared_ptr<UIWidget>& panel) {
        if (!edit || !panel)
            return;
        const Vector3 panelSize = panel->getTransform()->getSize();
        edit->getTransform()->setPosition(6.0f, 25.0f, 0.0f);
        edit->getTransform()->setSize(std::max(1.0f, panelSize.x - 12.0f), std::max(1.0f, panelSize.y - 31.0f));
        if (edit->getText() != text)
            edit->setText(text);
    };
    updateLog(log, panel);
    updateLog(m_shell.m_build->log, m_shell.m_build->panel);
}

}  // namespace morrow::editor
