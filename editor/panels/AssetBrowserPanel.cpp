#include "panels/AssetBrowserPanel.h"

#include "EditorShell.h"
#include "ui/FileSystemPanel.h"

namespace morrow::editor {
AssetBrowserPanel::AssetBrowserPanel(EditorShell& shell) : m_shell(shell) {
}

void AssetBrowserPanel::refresh() {
    std::string error;
    if (!m_shell.m_fileSystem.refresh(error)) {
        m_shell.setStatus("FileSystem refresh failed: " + error);
        return;
    }
    if (view)
        view->refreshView();
    m_shell.setStatus("FileSystem: " + std::to_string(m_shell.m_fileSystem.entries().size()) + " entries, " + std::to_string(m_shell.m_assets.assets().size()) + " assets");
}

void AssetBrowserPanel::importAssets() {
    std::string metadataError;
    if (!m_shell.m_assets.ensureImportMetadata(metadataError)) {
        m_shell.setStatus("Import metadata generation failed: " + metadataError);
        return;
    }

    std::string scanError;
    if (!m_shell.m_assets.scan(m_shell.m_projectPath.parent_path(), m_shell.m_assetRoot, scanError)) {
        m_shell.setStatus("Asset rescan failed: " + scanError);
        return;
    }

    std::vector<ImportTaskResult> results;
    std::string error;
    const bool success = m_shell.m_importQueue.importAll(m_shell.m_assets, m_shell.m_projectPath.parent_path(), "windows", results, error);
    size_t imported = 0;
    size_t skipped = 0;
    size_t failed = 0;
    for (const auto& result : results) {
        if (!result.success)
            ++failed;
        else if (result.skipped)
            ++skipped;
        else
            ++imported;
    }
    m_shell.setStatus("Assets: imported=" + std::to_string(imported) + " unchanged=" + std::to_string(skipped) + " failed=" + std::to_string(failed));
    if (!success && !error.empty())
        m_shell.setStatus("Import failed: " + error);
    if (!m_shell.m_assets.scan(m_shell.m_projectPath.parent_path(), m_shell.m_assetRoot, scanError)) {
        m_shell.setStatus("Asset rescan failed: " + scanError);
    }
    if (!m_shell.m_fileSystem.projectRoot().empty()) {
        if (!m_shell.m_fileSystem.refresh(scanError)) {
            m_shell.setStatus("FileSystem refresh failed: " + scanError);
        } else if (view) {
            view->refreshView();
        }
    }
    if (!results.empty()) {
        m_shell.m_events.onAssetDatabaseChanged.notify(m_shell.m_assets);
    }
}

}  // namespace morrow::editor
