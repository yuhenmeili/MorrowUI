#ifndef MORROW_EDITOR_RENAME_NODE_DIALOG_H
#define MORROW_EDITOR_RENAME_NODE_DIALOG_H

#include <memory>
#include <string>

#include "base/UIWidget.h"
#include "core/Observable.h"

namespace morrow {
class BaseButton;
class MRButton;
class MRLabel;
class MRLineEdit;
class MRTextEdit;
} // namespace morrow

namespace morrow::editor {
class RenameNodeDialog : public UIWidget {
public:
    struct Events {
        Observable<RenameNodeDialog&, const std::string&, const std::string&> onConfirmed;
    };

    static std::shared_ptr<RenameNodeDialog> create();

    void show(const std::string& nodeId, const std::string& currentName);

    void hideDialog();

    bool isOpen() const;

    Events& events();

    void update(FrameStateSharedPtr frameState) override;

private:
    RenameNodeDialog();

    void initializeControls();

    void layoutControls();

    void confirm();

    Events m_events;
    std::shared_ptr<UIWidget> m_backdrop;
    std::shared_ptr<UIWidget> m_panel;
    std::shared_ptr<MRLabel> m_title;
    std::shared_ptr<MRLineEdit> m_nameEdit;
    std::shared_ptr<MRButton> m_confirmButton;
    std::shared_ptr<MRButton> m_cancelButton;
    Observable<MRTextEdit&, const std::wstring&>::Connection m_submitConnection;
    Observable<BaseButton&>::Connection m_confirmConnection;
    Observable<BaseButton&>::Connection m_cancelConnection;
    std::string m_nodeId;
    bool m_open = false;
};
} // namespace morrow::editor

#endif