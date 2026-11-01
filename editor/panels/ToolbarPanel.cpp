#include "panels/ToolbarPanel.h"

#include <string>

#include "EditorShell.h"
#include "morrow/elements/MRButton.h"
#include "panels/AssetBrowserPanel.h"
#include "panels/BuildPanel.h"
#include "panels/InspectorPanel.h"
#include "panels/SceneTreePanel.h"
#include "panels/ViewportPanel.h"

namespace morrow::editor {

ToolbarPanel::ToolbarPanel(EditorShell& shell) : m_shell(shell) {
}

/// 工具栏按最窄 1036px framebuffer 排布；吸附开关/步长按钮在视口顶栏第二行
///（EditorShell 创建，文案由 EditorShell::refreshSnapButtons 维护）。
void ToolbarPanel::build() {
    m_shell.addLabel(panel, "MorrowEditor", 8.0f, 5.0f, 150.0f, 28.0f);
    m_shell.addButton(panel, L"Save", 170.0f, 4.0f, 66.0f, 30.0f, [this] {
        std::string error;
        if (m_shell.m_session->save(error))
            m_shell.setStatus("Saved");
        else
            m_shell.setStatus(error);
    });
    m_shell.addButton(panel, L"Undo", 240.0f, 4.0f, 66.0f, 30.0f, [this] {
        std::string error;
        if (m_shell.m_session->undo(error)) {
            m_shell.m_viewport->rebuildRuntime();
            m_shell.m_sceneTree->refresh();
            m_shell.m_inspector->refresh();
        }
        m_shell.setStatus(error.empty() ? "Undo" : error);
    });
    m_shell.addButton(panel, L"Redo", 310.0f, 4.0f, 66.0f, 30.0f, [this] {
        std::string error;
        if (m_shell.m_session->redo(error)) {
            m_shell.m_viewport->rebuildRuntime();
            m_shell.m_sceneTree->refresh();
            m_shell.m_inspector->refresh();
        }
        m_shell.setStatus(error.empty() ? "Redo" : error);
    });
    m_shell.addButton(panel, L"Configure", 380.0f, 4.0f, 78.0f, 30.0f, [this] { m_shell.m_build->run(BuildTaskKind::Configure); });
    m_shell.addButton(panel, L"Build", 462.0f, 4.0f, 70.0f, 30.0f, [this] { m_shell.m_build->run(BuildTaskKind::Build); });
    m_shell.addButton(panel, L"Build & Run", 536.0f, 4.0f, 104.0f, 30.0f, [this] { m_shell.m_build->run(BuildTaskKind::BuildAndRun); });
    m_shell.addButton(panel, L"Run Last", 644.0f, 4.0f, 86.0f, 30.0f, [this] { m_shell.m_build->run(BuildTaskKind::Run); });
    m_shell.addButton(panel, L"Stop", 734.0f, 4.0f, 64.0f, 30.0f, [this] { m_shell.m_build->stop(); });
    m_shell.addButton(panel, L"Import", 802.0f, 4.0f, 70.0f, 30.0f, [this] { m_shell.m_assetsPanel->importAssets(); });
    m_shell.addButton(panel, L"Assets", 876.0f, 4.0f, 72.0f, 30.0f, [this] { m_shell.m_assetsPanel->refresh(); });
}

}  // namespace morrow::editor
