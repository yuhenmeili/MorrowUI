#ifndef MORROW_EDITOR_INSPECTOR_PANEL_H
#define MORROW_EDITOR_INSPECTOR_PANEL_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "morrow/core/Observable.h"
#include "morrow/base/EventDispatcher.h"
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
    /// 属性搜索过滤词（Enter 提交生效）。
    std::string searchFilter;
    /// 已折叠的分组标题（会话内记忆）。
    std::set<std::string> collapsedComponents;
    std::shared_ptr<MRLineEdit> searchBox;
    EventConnection searchClickConnection;
    std::vector<Observable<BaseButton&>::Connection> headerConnections;
    std::string selectedAssetId;
    std::string selectedAssetType;
    bool m_applyingInspectorValue = false;

    void updateValues(const std::vector<InspectorProperty>& properties);

    /// 常驻搜索框（懒创建，资产视图隐藏）。
    void ensureSearchBox();

    /// 分组标题点击：切换折叠并重建（会话内记忆）。
    void toggleComponentCollapsed(const std::string& component);

    EditorShell& m_shell;
};
} // namespace morrow::editor

#endif  // MORROW_EDITOR_INSPECTOR_PANEL_H
