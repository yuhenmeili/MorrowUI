#ifndef MORROW_EDITOR_CREATE_NODE_DIALOG_H
#define MORROW_EDITOR_CREATE_NODE_DIALOG_H

#include <map>
#include <memory>
#include <string>

#include "base/UIWidget.h"
#include "core/Observable.h"
#include "scene/NodeTypeCatalog.h"

namespace morrow {
class BaseButton;
class MRButton;
class MRLabel;
class MRLineEdit;
class MRTextEdit;
class MRTree;
}  // namespace morrow

namespace morrow::editor {

class CreateNodeDialog : public UIWidget {
public:
    struct Events {
        Observable<CreateNodeDialog&, const NodeTypeDescriptor&, const std::string&> onConfirmed;
        Observable<CreateNodeDialog&> onCanceled;
    };

    static std::shared_ptr<CreateNodeDialog> create(const NodeTypeCatalog& catalog);

    void showForParent(const std::string& parentId, const std::string& parentName);

    void hideDialog();

    bool isOpen() const;

    Events& events();

    void update(FrameStateSharedPtr frameState) override;

private:
    explicit CreateNodeDialog(const NodeTypeCatalog& catalog);

    void initializeControls();
    void layoutControls();
    void rebuildTypes();
    void selectType(const std::string& type);
    void confirm();

    const NodeTypeCatalog& m_catalog;
    Events m_events;
    std::shared_ptr<UIWidget> m_backdrop;
    std::shared_ptr<UIWidget> m_panel;
    std::shared_ptr<MRLabel> m_titleLabel;
    std::shared_ptr<MRLabel> m_parentLabel;
    std::shared_ptr<MRLineEdit> m_searchEdit;
    std::shared_ptr<MRTree> m_typeTree;
    std::shared_ptr<MRLabel> m_descriptionTitle;
    std::shared_ptr<MRLabel> m_descriptionLabel;
    std::shared_ptr<MRButton> m_createButton;
    std::shared_ptr<MRButton> m_cancelButton;
    Observable<MRTextEdit&, const std::wstring&>::Connection m_searchConnection;
    Observable<MRTree&, int, const std::wstring&>::Connection m_treeConnection;
    Observable<BaseButton&>::Connection m_createConnection;
    Observable<BaseButton&>::Connection m_cancelConnection;
    std::map<int, std::string> m_typeByTreeId;
    std::string m_filter;
    std::string m_parentId;
    std::string m_parentName;
    std::string m_selectedType;
    bool m_open = false;
};

}  // namespace morrow::editor

#endif  // MORROW_EDITOR_CREATE_NODE_DIALOG_H
