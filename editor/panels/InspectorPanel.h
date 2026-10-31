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
class MRColor;
class MRLineEdit;
class MRPopupMenu;
class MRPopup;
class MRSelectableButton;
class MRSlider;
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
    std::shared_ptr<MRSlider> slider;
};

class InspectorPanel : public EditorPanel {
public:
    explicit InspectorPanel(EditorShell& shell);

    std::string assetPropertyAt(float x, float y, const AssetRecord& asset) const;

    void setAssetDropTarget(const std::string& property);

    void refresh(bool force = false);

    /// 应用属性值到全部选中节点；continuous=true 时连续编辑合并为单条撤销。
    void applyValue(const std::string& property, const std::string& value, bool continuous = false);

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
    /// 颜色选择器弹出（色板 MVP）。
    std::shared_ptr<MRPopup> colorPopup;
    std::shared_ptr<UIWidget> colorPopupContent;
    std::string colorEditProperty;
    float colorEditAlpha = 1.0f;
    /// 最近使用的颜色（"Color(r, g, b, a)" 序列化形态，最多 8 个）。
    std::vector<std::string> recentColors;
    std::vector<EventConnection> colorPopupConnections;
    /// number 范围属性的滑块连接（随 refresh 重建）。
    std::vector<Observable<MRSlider&, float>::Connection> sliderConnections;
    std::string selectedAssetId;
    std::string selectedAssetType;
    bool m_applyingInspectorValue = false;

    void updateValues(const std::vector<InspectorProperty>& properties);

    /// 常驻搜索框（懒创建，资产视图隐藏）。
    void ensureSearchBox();

    /// 分组标题点击：切换折叠并重建（会话内记忆）。
    void toggleComponentCollapsed(const std::string& component);

    /// 打开取色器弹出（世界坐标定位）。
    void openColorPicker(const InspectorProperty& property, float worldX, float worldY);

    /// 懒建取色器弹窗骨架（预设色板 + 当前色预览 + 最近使用行）。
    void ensureColorPopup();

    /// 记录最近使用的颜色（去重置顶，上限 8）。
    void pushRecentColor(const std::string& value);

    EditorShell& m_shell;
};
} // namespace morrow::editor

#endif  // MORROW_EDITOR_INSPECTOR_PANEL_H
