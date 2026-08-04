# ButtonDemo

- 对应源码：`samples/ButtonDemo.cpp`
- 编译目标：`ButtonDemo`

## Demo 用途

`ButtonDemo` 展示 `MRButton` 与布局容器的组合方式。当前源码实际启用了一个 `HBoxContainer`，把两个按钮按水平方向排列；同时保留了 `CenterContainer` 和 `MarginContainer` 的示例代码，适合作为布局系统入门参考。

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target ButtonDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\ButtonDemo.exe"
```

## 示例做了什么

1. 创建启用调试参数的 `Engine("win", true)`。
2. 注册默认字体 `MorrowSansCN1.1-Regular.otf`。
3. 创建两个 `MRButton`：
   - `button1`：默认外观，`100 x 100`
   - `button2`：红色背景，`200 x 200`
4. 创建 `HBoxContainer`，设置位置、尺寸和 `spacing = 8`。
5. 把两个按钮加入容器，再把容器加入窗口。

## 相关组件介绍

### `MRButton`
- 基于 `BaseButton` 的文本按钮组件。
- 支持文本、背景色、hover/pressed/disabled 状态色、边框和圆角等能力。
- 通过 `setText()` 可把文字渲染逻辑委托给内部 `MRLabel`。

### `BaseButton`
- 负责按钮通用交互状态管理。
- 提供 `setOnClickCallback()`、`setOnHoverCallback()` 等回调绑定能力。
- 适合做所有“可点击 UI 控件”的父类基础。

### `HBoxContainer`
- 一个水平布局容器。
- 子节点从左到右排列，可通过 `setSpacing()` 控制间距。
- 在列表式按钮、设置项入口等场景很常用。

### `CenterContainer`
- 用于把子节点在容器内居中。
- 适合单个主按钮、弹窗主图标等场景。
- 本 demo 中相关代码被注释掉，可按需打开实验。

### `MarginContainer`
- 用于给唯一子节点添加内边距。
- 适合卡片内容区、按钮内边距、表单区布局。

## 资源依赖

- 字体：`assets/fonts/MorrowSansCN1.1-Regular.otf`

## 适合继续扩展的方向

- 为两个按钮补充点击回调和状态切换日志。
- 把 `HBoxContainer`、`CenterContainer`、`MarginContainer` 组合成一个完整布局演示页面。
- 增加 `VBoxContainer` 示例，对比水平和垂直布局行为。

