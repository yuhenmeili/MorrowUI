# ListNavigationDemo

- 对应源码：`samples/ListNavigationDemo.cpp`
- 编译目标：`ListNavigationDemo`

## Demo 用途

列表与导航类控件集中展示：`MRItemList`（单选列表）、`MRTree`（树形展开/折叠）、
`MRScrollContainer`（长列表滚动），三个面板并排，选择结果回显到各自底部状态标签。

## 运行方式

```bash
cmake --build build --target ListNavigationDemo --parallel 8
./build/ListNavigationDemo.exe
```

## 示例做了什么

1. **MRItemList 面板**：`setItemHeight(48)`，`addItem(text, id)` 添加 10 项，其中一项
   `enabled = false`（"赛道模式"不可选）；`events().onItemSelected(id, text)` 回调更新状态标签。
2. **MRTree 面板**：`addNode(id, text, parentId)` 构建三级车辆设置树，含不可用节点；
   `events().onNodeSelected` 更新状态标签，`events().onNodeExpanded` 打印展开/折叠。
3. **MRScrollContainer 面板**：`setScrollStep(56)`，自定义垂直滚动条颜色
   （`getVerticalScrollBar()->setTrackColor/setThumbColor`），`addScrollChild` 放入
   24 个按钮行，滚轮/拖动/滚动条三种方式滚动。

## 相关组件

### `MRItemList`
- 扁平单选列表：`addItem(text, id, enabled)`、`setItemHeight`、
  `events().onItemSelected`。

### `MRTree`
- 树形列表：`addNode(id, text, parentId = -1, expanded = true, enabled = true)`；
  行样式可通过 `setRowStyle(RowStyle)` 全量定制（选中/悬停/按下配色）。

### `MRScrollContainer`
- 滚动容器：`setContent/addScrollChild` 放入超出视口的内容，`setScrollStep` 控制滚轮步长，
  `getVerticalScrollBar()` 可定制滚动条外观。
