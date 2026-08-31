#ifndef MORROW_EDITOR_INSPECTOR_PANEL_H
#define MORROW_EDITOR_INSPECTOR_PANEL_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/Observable.h"
#include "panels/EditorPanel.h"

namespace morrow {
class BaseButton;
class MRButton;
class MRLineEdit;
class MRPopupMenu;
class MRTextEdit;
class TouchEvent;
class UIWidget;
} // namespace morrow

namespace morrow::editor {
class EditorShell;
struct AssetRecord;
struct InspectorProperty;

struct InspectorBinding {
    std::string property;
    std::string type;
    std::vector<std::shared_ptr<MRLineEdit>> edits;
    std::shared_ptr<MRButton> button;
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

private:
    friend class EditorShell;

    std::shared_ptr<UIWidget>& panel = m_root;
    std::shared_ptr<MRPopupMenu> assetMenu;
    std::vector<Observable<MRTextEdit&, const std::wstring&>::Connection> editConnections;
    std::map<std::string, InspectorBinding> bindings;
    std::map<int, std::string> assetMenuIds;
    Observable<MRPopupMenu&, int, const std::wstring&>::Connection assetMenuConnection;
    std::string assetEditProperty;
    std::vector<std::string> assetEditNodeIds;
    std::string assetDropProperty;
    std::string legacyEditProperty;
    std::string legacyEditValue;
    std::string lastSchemaSignature;

    void updateValues(const std::vector<InspectorProperty>& properties);

    EditorShell& m_shell;
};
} // namespace morrow::editor

#endif  // MORROW_EDITOR_INSPECTOR_PANEL_H
