#ifndef MORROW_EDITOR_FILE_SYSTEM_PANEL_H
#define MORROW_EDITOR_FILE_SYSTEM_PANEL_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/UIWidget.h"
#include "core/Observable.h"
#include "elements/MRTree.h"
#include "filesystem/ProjectFileSystemModel.h"

namespace morrow {
class BaseButton;
class MRButton;
class MRLabel;
class MRLineEdit;
class MRTextEdit;
}  // namespace morrow

namespace morrow::editor {

class FileSystemPanel : public UIWidget {
public:
    static std::shared_ptr<FileSystemPanel> create(
        ProjectFileSystemModel& model,
        std::function<void(const std::string&)> statusCallback = {});

    void refreshView();

    void update(FrameStateSharedPtr frameState) override;

private:
    FileSystemPanel(
        ProjectFileSystemModel& model,
        std::function<void(const std::string&)> statusCallback);

    void initializeControls();
    void layoutControls();
    void rebuildTree();
    void refreshModel();
    void handleSelection(int id);
    std::wstring entryText(const ProjectFileEntry& entry) const;
    void setPanelStatus(const std::string& status);

    ProjectFileSystemModel& m_model;
    std::function<void(const std::string&)> m_statusCallback;
    std::shared_ptr<MRLabel> m_titleLabel;
    std::shared_ptr<MRButton> m_refreshButton;
    std::shared_ptr<MRLineEdit> m_searchEdit;
    std::shared_ptr<MRLabel> m_pathLabel;
    std::shared_ptr<MRTree> m_tree;
    std::shared_ptr<MRLabel> m_statusLabel;
    Observable<BaseButton&>::Connection m_refreshConnection;
    Observable<MRTextEdit&, const std::wstring&>::Connection
        m_searchConnection;
    Observable<MRTree&, int, const std::wstring&>::Connection
        m_selectionConnection;
    std::string m_filter;
    std::string m_status = "Ready";
};

}  // namespace morrow::editor

#endif  // MORROW_EDITOR_FILE_SYSTEM_PANEL_H
