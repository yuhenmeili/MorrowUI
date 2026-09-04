#ifndef MORROW_EDITOR_INSPECTOR_PANEL_H
#define MORROW_EDITOR_INSPECTOR_PANEL_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/Observable.h"
#include "base/EventDispatcher.h"
#include "panels/EditorPanel.h"

namespace morrow {
class BaseButton;
class MRButton;
class MRCheckBox;
class MRLineEdit;
class MRPopupMenu;
class MRSelectableButton;
class MRTextEdit;
class TouchEvent;
class UIWidget;
class Widget;
} // namespace morrow

namespace morrow::editor {
class EditorShell;
struct AssetRecord;
struct InspectorProperty;
struct ProjectFileEntry;

struct InspectorBinding {
    std::string property;
    std::string type;
    std::vector<std::shared_ptr<MRLineEdit>> edits;
    std::shared_ptr<MRButton> button;
    std::shared_ptr<MRLineEdit> resourceField;
    std::shared_ptr<Widget> dropTarget;
    std::shared_ptr<MRCheckBox> checkBox;
};

class InspectorPanel : public EditorPanel {
public:
    explicit InspectorPanel(EditorShell& shell);

    std::string assetPropertyAt(float x, float y, const AssetRecord& asset) const;

    void setAssetDropTarget(const std::string& property);

    void refresh(bool force = false);

    void applyValue(const std::string& property, const std::string& value);

    void beginLegacyPropertyEdit(const std::string& property, const std::string& value);

    void commitLegacyPropertyEdit();

    void appendLegacyCharacter(unsigned int codepoint);

    void inspectAsset(const ProjectFileEntry& entry);

    void clearAsset();

private:
    friend class EditorShell;

    std::shared_ptr<UIWidget>& panel = m_root;
    std::shared_ptr<MRPopupMenu> assetMenu;
    std::vector<Observable<MRTextEdit&, const std::wstring&>::Connection> editConnections;
    std::vector<Observable<MRSelectableButton&, bool>::Connection> checkConnections;
    std::vector<EventConnection> interactionConnections;
    std::map<std::string, InspectorBinding> bindings;
    std::map<int, std::string> assetMenuIds;
    Observable<MRPopupMenu&, int, const std::wstring&>::Connection assetMenuConnection;
    std::string assetEditProperty;
    std::vector<std::string> assetEditNodeIds;
    std::string assetDropProperty;
    std::string legacyEditProperty;
    std::string legacyEditValue;
    std::string lastSchemaSignature;
    std::string selectedAssetId;
    std::string selectedAssetType;
    bool m_applyingInspectorValue = false;

    void updateValues(const std::vector<InspectorProperty>& properties);

    EditorShell& m_shell;
};
} // namespace morrow::editor

#endif  // MORROW_EDITOR_INSPECTOR_PANEL_H
