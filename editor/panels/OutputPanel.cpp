#include "panels/OutputPanel.h"

#include <algorithm>
#include <codecvt>
#include <locale>

#include "EditorShell.h"
#include "morrow/base/Transform.h"
#include "morrow/elements/MRLineEdit.h"
#include "morrow/elements/MRTextEdit.h"
#include "panels/BuildPanel.h"

namespace {
/// UTF-8 → wstring：codecvt 优先，失败退回逐字符（纯 ASCII 恒等）。
std::wstring wide(const std::string& text) {
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(text);
    } catch (...) {
        return std::wstring(text.begin(), text.end());
    }
}
}  // namespace

namespace morrow::editor {
OutputPanel::OutputPanel(EditorShell& shell) : m_shell(shell) {
}

void OutputPanel::append(const std::string& line) {
    lines.push_back(line);
    while (lines.size() > 500)
        lines.erase(lines.begin());
    m_shell.m_outputLines = lines;
    refresh();
}

void OutputPanel::resize() {
    refresh();
}

void OutputPanel::clear() {
    lines.clear();
    m_shell.m_outputLines.clear();
    refresh();
}

void OutputPanel::setFilter(const std::string& filter) {
    m_filter = filter;
    if (filterEdit)
        filterEdit->setText(wide(filter));
    refresh();
}

void OutputPanel::refresh() {
    if (!panel || !m_shell.m_build->panel)
        return;
    std::wstring text;
    lines = m_shell.m_outputLines;
    for (size_t index = 0; index < lines.size(); ++index) {
        if (!m_filter.empty() && lines[index].find(m_filter) == std::string::npos)
            continue;
        if (!text.empty())
            text.push_back(L'\n');
        text += wide(lines[index]);
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
