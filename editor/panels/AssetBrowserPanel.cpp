#include "panels/AssetBrowserPanel.h"

#include <fstream>

#include "EditorShell.h"
#include "elements/MRPopupMenu.h"
#include "ui/CreateAssetDialog.h"
#include "ui/RenameNodeDialog.h"
#include "FileSystemPanel.h"

namespace morrow::editor {
AssetBrowserPanel::AssetBrowserPanel(EditorShell& shell) : m_shell(shell) {
}

void AssetBrowserPanel::showCreateDialog() {
    if (createAssetDialog)
        createAssetDialog->show();
}

void AssetBrowserPanel::showContextMenu(const ProjectFileEntry* entry, float x, float y) {
    contextPath = entry ? entry->relativePath : view->currentDirectory();
    contextMenu->attachTo(m_shell.m_shellRoot);
    contextMenu->popup(x, y);
}

void AssetBrowserPanel::createFolder() {
    contextPath = view ? view->currentDirectory() : std::filesystem::path{"."};
    nameDialog->show("create_folder", "NewFolder");
}

void AssetBrowserPanel::renameSelected() {
    if (contextPath.empty() || contextPath == ".") {
        m_shell.setStatus("Select a file or folder to rename");
        return;
    }
    nameDialog->show("rename", contextPath.filename().string());
}

void AssetBrowserPanel::deleteSelected() {
    deletePaths({contextPath});
}

void AssetBrowserPanel::deletePaths(const std::vector<std::filesystem::path>& relativePaths) {
    std::vector<std::filesystem::path> targets;
    for (const auto& relativePath : relativePaths) {
        if (!relativePath.empty() && relativePath != ".")
            targets.push_back(relativePath);
    }
    if (targets.empty()) {
        m_shell.setStatus("Select a file or folder to delete");
        return;
    }
    std::error_code error;
    for (const auto& relativePath : targets) {
        const auto absolute = m_shell.m_fileSystem.projectRoot() / relativePath;
        if (std::filesystem::is_directory(absolute, error))
            std::filesystem::remove_all(absolute, error);
        else {
            std::filesystem::remove(absolute, error);
            std::filesystem::remove(absolute.string() + ".import", error);
        }
        if (error)
            break;
    }
    if (error) {
        m_shell.setStatus("Delete failed: " + error.message());
        return;
    }
    refresh();
    m_shell.setStatus(targets.size() == 1 ? "Deleted " + targets.front().generic_string() : "Deleted " + std::to_string(targets.size()) + " entries");
    contextPath.clear();
}

void AssetBrowserPanel::applyNameDialog(const std::string& action, const std::string& name) {
    if (name.empty())
        return;
    std::error_code error;
    if (action == "create_folder") {
        const auto base = m_shell.m_fileSystem.projectRoot() / (contextPath.empty() ? std::filesystem::path{"."} : contextPath);
        std::filesystem::create_directories(base / name, error);
        if (error)
            m_shell.setStatus("Create folder failed: " + error.message());
        else {
            refresh();
            m_shell.setStatus("Created folder " + name);
        }
        return;
    }
    if (action == "rename") {
        const auto source = m_shell.m_fileSystem.projectRoot() / contextPath;
        const auto target = source.parent_path() / name;
        std::filesystem::rename(source, target, error);
        if (!error && std::filesystem::exists(source.string() + ".import"))
            std::filesystem::rename(source.string() + ".import", target.string() + ".import", error);
        if (error)
            m_shell.setStatus("Rename failed: " + error.message());
        else {
            contextPath = std::filesystem::relative(target, m_shell.m_fileSystem.projectRoot(), error);
            refresh();
            m_shell.setStatus("Renamed to " + name);
        }
    }
}

void AssetBrowserPanel::createAsset(const AssetTypeDescriptor& descriptor, const std::string& name) {
    std::filesystem::path directory = m_shell.m_projectPath.parent_path() / m_shell.m_assetRoot;
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        m_shell.setStatus("Failed to create asset directory: " + error.message());
        return;
    }
    std::filesystem::path path = directory / name;
    path.replace_extension(descriptor.extension);
    if (std::filesystem::exists(path)) {
        m_shell.setStatus("Asset already exists: " + path.filename().string());
        return;
    }
    std::ofstream output(path);
    if (descriptor.type == "Material") {
        output << "[material]\nformat = 1\nshader = \"\"\n\n[properties]\ncolor = Color(1.0, 1.0, 1.0, 1.0)\n";
    } else if (descriptor.type == "Shader") {
        output << "#version 330 core\n\n";
    }
    if (!output.good()) {
        m_shell.setStatus("Failed to create asset: " + path.string());
        return;
    }
    std::string scanError;
    if (!m_shell.m_assets.ensureImportMetadata(scanError) || !m_shell.m_assets.scan(m_shell.m_projectPath.parent_path(), m_shell.m_assetRoot, scanError)) {
        m_shell.setStatus("Asset created, but scan failed: " + scanError);
        return;
    }
    if (view)
        view->refreshView();
    m_shell.setStatus("Created " + path.filename().string());
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
