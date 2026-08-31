#ifndef MORROW_EDITOR_CREATE_ASSET_DIALOG_H
#define MORROW_EDITOR_CREATE_ASSET_DIALOG_H

#include <memory>
#include <map>
#include <string>

#include "base/UIWidget.h"
#include "core/Observable.h"
#include "assets/AssetTypeCatalog.h"

namespace morrow {
class BaseButton;
class MRButton;
class MRLineEdit;
class MRTree;
}

namespace morrow::editor {
class CreateAssetDialog : public UIWidget {
public:
    struct Events {
        Observable<CreateAssetDialog&, const AssetTypeDescriptor&, const std::string&> onConfirmed;
        Observable<CreateAssetDialog&> onCanceled;
    };

    static std::shared_ptr<CreateAssetDialog> create();

    void show();

    void hideDialog();

    bool isOpen() const;

    Events& events();

    void update(FrameStateSharedPtr state) override;

private:
    CreateAssetDialog();

    void initializeControls();

    void confirm();

    Events m_events;
    std::shared_ptr<UIWidget> m_panel;
    std::shared_ptr<MRLineEdit> m_name;
    std::map<int, int> m_typeIds;
    std::shared_ptr<MRTree> m_types;
    std::shared_ptr<MRButton> m_create;
    std::shared_ptr<MRButton> m_cancel;
    Observable<BaseButton&>::Connection m_createConnection;
    Observable<BaseButton&>::Connection m_cancelConnection;
    Observable<MRTree&, int, const std::wstring&>::Connection m_typeConnection;
    std::string m_selectedType = "Material";
    bool m_open = false;
};
}

#endif
