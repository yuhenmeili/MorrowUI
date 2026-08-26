#ifndef MORROW_EDITOR_FILE_SYSTEM_PANEL_H
#define MORROW_EDITOR_FILE_SYSTEM_PANEL_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/UIWidget.h"
#include "core/Observable.h"
#include "elements/MRTree.h"
#include "filesystem/FileSystemWatcher.h"
#include "filesystem/ProjectFileSystemModel.h"
#include "filesystem/ThumbnailService.h"

namespace morrow {
class BaseButton;
class MRButton;
class MRLabel;
class MRLineEdit;
class MRImage;
class MRScrollContainer;
class MRTextEdit;
} // namespace morrow

namespace morrow::editor {
class FileSystemPanel : public UIWidget {
public:
    static std::shared_ptr<FileSystemPanel> create(ProjectFileSystemModel& model, std::function<void(const std::string&)> statusCallback = {});

    void refreshView();

    void update(FrameStateSharedPtr frameState) override;

private:
    FileSystemPanel(ProjectFileSystemModel& model, std::function<void(const std::string&)> statusCallback);

    void initializeControls();

    void layoutControls();

    void rebuildTree();

    void rebuildGrid();

    void refreshModel();

    void handleSelection(int id);

    void handleGridClick(int id, uint32_t modifiers);

    void cycleSort();

    void toggleView();

    void navigateBack();

    void updateCardStyles();

    std::wstring entryText(const ProjectFileEntry& entry) const;

    void setPanelStatus(const std::string& status);

    ProjectFileSystemModel& m_model;
    std::function<void(const std::string&)> m_statusCallback;
    std::shared_ptr<MRLabel> m_titleLabel;
    std::shared_ptr<MRButton> m_refreshButton;
    std::shared_ptr<MRButton> m_sortButton;
    std::shared_ptr<MRButton> m_viewButton;
    std::shared_ptr<MRButton> m_backButton;
    std::shared_ptr<MRLineEdit> m_searchEdit;
    std::shared_ptr<MRLabel> m_pathLabel;
    std::shared_ptr<MRTree> m_tree;
    std::shared_ptr<MRScrollContainer> m_gridScroll;
    std::shared_ptr<UIWidget> m_gridContent;
    std::shared_ptr<MRLabel> m_statusLabel;
    Observable<BaseButton&>::Connection m_refreshConnection;
    Observable<BaseButton&>::Connection m_sortConnection;
    Observable<BaseButton&>::Connection m_viewConnection;
    Observable<BaseButton&>::Connection m_backConnection;
    Observable<MRTextEdit&, const std::wstring&>::Connection m_searchConnection;
    Observable<MRTree&, int, const std::wstring&>::Connection m_selectionConnection;
    std::vector<EventConnection> m_gridConnections;
    std::vector<std::pair<int, std::shared_ptr<MRButton>>> m_gridCards;
    std::vector<std::pair<std::filesystem::path, std::shared_ptr<MRImage>>> m_gridImages;
    FileSystemWatcher m_watcher;
    ThumbnailService m_thumbnails;
    std::filesystem::path m_currentDirectory = ".";
    bool m_gridMode = false;
    std::string m_filter;
    std::string m_status = "Ready";
};
} // namespace morrow::editor

#endif  // MORROW_EDITOR_FILE_SYSTEM_PANEL_H