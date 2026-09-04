#ifndef MORROW_EDITOR_ASSET_BROWSER_PANEL_H
#define MORROW_EDITOR_ASSET_BROWSER_PANEL_H

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "filesystem/ProjectFileSystemModel.h"
#include "panels/EditorPanel.h"
#include "assets/AssetTypeCatalog.h"
#include "core/Observable.h"

namespace morrow {
class UIWidget;
class MRPopupMenu;
}

namespace morrow::editor {
class EditorShell;
class FileSystemPanel;
class CreateAssetDialog;
class RenameNodeDialog;

class AssetBrowserPanel : public EditorPanel {
public:
    explicit AssetBrowserPanel(EditorShell& shell);

    void refresh();

    void importAssets();

    void showCreateDialog();

    void createAsset(const AssetTypeDescriptor& descriptor, const std::string& name);

    void showContextMenu(const ProjectFileEntry* entry, float x, float y);

    void createFolder();

    void renameSelected();

    void deleteSelected();

    void deletePaths(const std::vector<std::filesystem::path>& relativePaths);

    void applyNameDialog(const std::string& action, const std::string& name);

    std::shared_ptr<FileSystemPanel> view;
    std::shared_ptr<CreateAssetDialog> createAssetDialog;
    std::shared_ptr<RenameNodeDialog> nameDialog;
    std::shared_ptr<MRPopupMenu> contextMenu;
    Observable<CreateAssetDialog&, const AssetTypeDescriptor&, const std::string&>::Connection assetCreateConnection;
    Observable<MRPopupMenu&, int, const std::wstring&>::Connection contextMenuConnection;
    Observable<RenameNodeDialog&, const std::string&, const std::string&>::Connection nameDialogConnection;
    std::filesystem::path contextPath;

private:
    friend class EditorShell;
    EditorShell& m_shell;
};
} // namespace morrow::editor

#endif