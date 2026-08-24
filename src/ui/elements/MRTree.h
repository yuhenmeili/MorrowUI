#ifndef MORROW_GUI_MRTREE_H
#define MORROW_GUI_MRTREE_H

#include <memory>
#include <string>
#include <vector>

#include "MRButton.h"
#include "base/UIWidget.h"

namespace morrow {

/// 支持展开、折叠和单选的树形列表控件。
class MRTree : public UIWidget {
public:
    struct RowStyle {
        Vector4 textColor = Vector4(0.12f, 0.16f, 0.22f, 1.0f);
        Vector4 backgroundColor = Vector4(0.91f, 0.94f, 0.92f, 1.0f);
        Vector4 hoverColor = Vector4(0.82f, 0.89f, 0.85f, 1.0f);
        Vector4 selectedTextColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        Vector4 selectedBackgroundColor = Vector4(0.16f, 0.5f, 0.42f, 1.0f);
        Vector4 selectedHoverColor = Vector4(0.12f, 0.43f, 0.36f, 1.0f);
        Vector4 pressedColor = Vector4(0.09f, 0.36f, 0.31f, 1.0f);
    };

    struct Events {
        Observable<MRTree&, int, const std::wstring&> onNodeSelected;
        Observable<MRTree&, int, bool> onNodeExpanded;
    };

    /// 树节点数据。
    struct Node {
        /// 节点标识。
        int id = 0;
        /// 父节点标识；-1 表示根节点。
        int parentId = -1;
        /// 节点显示文字。
        std::wstring text;
        /// 节点是否展开。
        bool expanded = true;
        /// 节点是否可用。
        bool enabled = true;
    };

    /// 创建树形控件。
    static std::shared_ptr<MRTree> create();
    /// 添加节点；parentId 为 -1 时添加根节点。
    bool addNode(int id, const std::wstring& text, int parentId = -1, bool expanded = true, bool enabled = true);
    /// 删除全部节点。
    void clear();
    /// 设置每个可见节点的行高。
    void setRowHeight(float height);
    /// 设置每一级树节点的缩进宽度。
    void setIndentWidth(float width);
    /// 设置树行的正常、悬停和选中颜色。
    void setRowStyle(const RowStyle& style);
    /// 设置垂直滚动偏移。
    void setScrollOffset(float offset);
    /// 展开或折叠指定节点。
    bool setExpanded(int id, bool expanded);
    /// 切换指定节点的展开状态。
    bool toggleExpanded(int id);
    /// 查询指定节点是否展开。
    bool isExpanded(int id) const;
    /// 选中指定节点。
    bool selectNode(int id);
    /// 获取当前选中节点 id；未选中时返回 -1。
    int getSelectedId() const;

    Events& events();
    /// 获取只读树节点数据。
    const std::vector<Node>& getNodes() const;
    /// 每帧刷新可见节点、滚动范围和行布局。
    void update(FrameStateSharedPtr frameState) override;

private:
    struct VisibleNode {
        size_t nodeIndex = 0;
        int depth = 0;
    };

    MRTree();

    int findNodeIndex(int id) const;

    bool hasChildren(int id) const;

    void rebuildVisibleNodes();

    void appendVisibleChildren(int parentId, int depth);

    void layoutRows();

    void selectIndex(size_t nodeIndex, bool notify);

    void attachWheel(const std::shared_ptr<Widget>& widget);

    void configureRow(size_t rowIndex, const VisibleNode& visibleNode);

    std::vector<Node> m_nodes;
    std::vector<VisibleNode> m_visibleNodes;
    std::vector<std::shared_ptr<MRButton>> m_rows;
    std::vector<Observable<BaseButton&>::Connection> m_rowClickConnections;
    std::vector<EventConnection> m_wheelConnections;
    Events m_events;
    float m_rowHeight = 46.0f;
    float m_indentWidth = 28.0f;
    float m_scrollOffset = 0.0f;
    float m_maxScrollOffset = 0.0f;
    int m_selectedIndex = -1;
    bool m_treeDirty = true;
    RowStyle m_rowStyle;
};

using MRTreeSharedPtr = std::shared_ptr<MRTree>;
using Tree = MRTree;

}  // namespace morrow

#endif  // MORROW_GUI_MRTREE_H
