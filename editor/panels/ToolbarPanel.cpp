#include "panels/ToolbarPanel.h"

#include <string>

#include "EditorShell.h"
#include "morrow/elements/MRButton.h"
#include "panels/AssetBrowserPanel.h"
#include "panels/BuildPanel.h"
#include "panels/InspectorPanel.h"
#include "panels/SceneTreePanel.h"
#include "panels/ViewportPanel.h"

namespace {
const float kSnapSteps[] = {1.0f, 2.0f, 5.0f, 10.0f, 20.0f, 50.0f};
}  // namespace

namespace morrow::editor {

ToolbarPanel::ToolbarPanel(EditorShell& shell) : m_shell(shell) {
}

void ToolbarPanel::refreshSnapButtons() {
    if (snapButton)
        snapButton->setText(m_shell.m_viewport->snapEnabled ? L"Snap: On" : L"Snap: Off", "default");
    if (stepButton)
        stepButton->setText(L"Step: " + std::to_wstring(static_cast<int>(m_shell.m_viewport->snapStep)), "default");
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
    snapButton = m_shell.addButton(panel, L"Snap: On", 1082.0f, 4.0f, 96.0f, 30.0f, [this] {
        m_shell.m_viewport->snapEnabled = !m_shell.m_viewport->snapEnabled;
        m_shell.m_viewport->refreshGuides();
        refreshSnapButtons();
        m_shell.setStatus(m_shell.m_viewport->snapEnabled ? "Snapping enabled" : "Snapping disabled");
    });
    stepButton = m_shell.addButton(panel, L"Step: 10", 1184.0f, 4.0f, 92.0f, 30.0f, [this] {
        float& step = m_shell.m_viewport->snapStep;
        for (size_t index = 0; index < sizeof(kSnapSteps) / sizeof(kSnapSteps[0]); ++index) {
            if (kSnapSteps[index] == step) {
                step = kSnapSteps[(index + 1) % (sizeof(kSnapSteps) / sizeof(kSnapSteps[0]))];
                break;
            }
        }
        m_shell.m_viewport->refreshGuides();
        refreshSnapButtons();
        m_shell.setStatus("Snap step " + std::to_string(static_cast<int>(step)) + "px");
    });
    refreshSnapButtons();
}

}  // namespace morrow::editor
