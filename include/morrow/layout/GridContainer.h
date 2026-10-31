//
// 网格布局容器：子节点按列数填入单元格，单元格均分容器尺寸并留间距。
// 局部坐标系左上角为 (0,0)，与 HBox/VBox 一致。
//

#ifndef GRIDCONTAINER_H
#define GRIDCONTAINER_H

#include "morrow/base/UIWidget.h"

namespace morrow {

class GridContainer : public UIWidget {
public:
    GridContainer();

    /// 列数（>=1）；行数由子节点数自动向上取整。
    void setColumns(int columns);

    int getColumns() const { return m_columns; }

    /// 固定行数（<=0 表示按子节点数自动）。
    void setRows(int rows);

    int getRows() const { return m_rows; }

    /// 单元格水平/垂直间距（同一值）。
    void setSpacing(float spacing);

    float getSpacing() const { return m_spacing; }

    void update(FrameStateSharedPtr frameState) override;

private:
    void layoutChildren();

    int m_columns = 2;
    int m_rows = 0;
    float m_spacing = 8.0f;
};

} // namespace morrow

#endif // GRIDCONTAINER_H
