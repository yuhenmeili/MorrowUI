#include "panels/ToolbarPanel.h"

#include "EditorShell.h"
#include "panels/AssetBrowserPanel.h"
#include "panels/BuildPanel.h"
#include "panels/InspectorPanel.h"
#include "panels/SceneTreePanel.h"
#include "panels/ViewportPanel.h"

namespace morrow::editor {

ToolbarPanel::ToolbarPanel(EditorShell& shell) : m_shell(shell) {
}

void ToolbarPanel::build() {
    m_shell.addLabel(panel, "MorrowEditor", 8.0f, 5.0f, 150.0f, 28.0f);
    m_shell.addButton(panel, L"Save", 170.0f, 4.0f, 72.0f, 30.0f, [this] {
        std::string error;
        if (m_shell.m_session->save(error))
            m_shell.setStatus("Saved");
        else
            m_shell.setStatus(error);
    });
    m_shell.addButton(panel, L"Undo", 248.0f, 4.0f, 72.0f, 30.0f, [this] {
        std::string error;
        if (m_shell.m_session->undo(error)) {
            m_shell.m_viewport->rebuildRuntime();
            m_shell.m_sceneTree->refresh();
            m_shell.m_inspector->refresh();
        }
        m_shell.setStatus(error.empty() ? "Undo" : error);
    });
    m_shell.addButton(panel, L"Redo", 326.0f, 4.0f, 72.0f, 30.0f, [this] {
        std::string error;
        if (m_shell.m_session->redo(error)) {
            m_shell.m_viewport->rebuildRuntime();
            m_shell.m_sceneTree->refresh();
            m_shell.m_inspector->refresh();
        }
        m_shell.setStatus(error.empty() ? "Redo" : error);
    });
    m_shell.addButton(panel, L"Configure", 420.0f, 4.0f, 100.0f, 30.0f, [this] { m_shell.m_build->run(BuildTaskKind::Configure); });
    m_shell.addButton(panel, L"Build", 526.0f, 4.0f, 80.0f, 30.0f, [this] { m_shell.m_build->run(BuildTaskKind::Build); });
    m_shell.addButton(panel, L"Build & Run", 612.0f, 4.0f, 112.0f, 30.0f, [this] { m_shell.m_build->run(BuildTaskKind::BuildAndRun); });
    m_shell.addButton(panel, L"Run Last", 730.0f, 4.0f, 92.0f, 30.0f, [this] { m_shell.m_build->run(BuildTaskKind::Run); });
    m_shell.addButton(panel, L"Stop", 828.0f, 4.0f, 72.0f, 30.0f, [this] { m_shell.m_build->stop(); });
    m_shell.addButton(panel, L"Import", 906.0f, 4.0f, 82.0f, 30.0f, [this] { m_shell.m_assetsPanel->importAssets(); });
    m_shell.addButton(panel, L"Assets", 994.0f, 4.0f, 82.0f, 30.0f, [this] { m_shell.m_assetsPanel->refresh(); });
}

}  // namespace morrow::editor
