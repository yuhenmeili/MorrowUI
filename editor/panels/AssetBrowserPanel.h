#ifndef MORROW_EDITOR_ASSET_BROWSER_PANEL_H
#define MORROW_EDITOR_ASSET_BROWSER_PANEL_H

#include <memory>
#include <string>

#include "filesystem/ProjectFileSystemModel.h"

namespace morrow {
class UIWidget;
}

namespace morrow::editor {
class EditorShell;
class FileSystemPanel;

class AssetBrowserPanel {
public:
    explicit AssetBrowserPanel(EditorShell& shell);

    void refresh();

    void importAssets();

    std::shared_ptr<FileSystemPanel> view;

private:
    friend class EditorShell;
    EditorShell& m_shell;
};
} // namespace morrow::editor

#endif