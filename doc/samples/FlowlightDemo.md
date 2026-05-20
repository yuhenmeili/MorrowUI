# FlowlightDemo

- 对应源码：`samples/FlowlightDemo.cpp`
- 编译目标：`FlowlightDemo`

## Demo 用途

`FlowlightDemo` 用来展示一类“扫光/流光”效果组件。示例非常轻量：只创建组件、设置尺寸和位置、调用 `initialize()`，适合作为 shader 型 UI 特效组件的最小接入范例。

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target FlowlightDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\FlowlightDemo.exe"
```

## 示例做了什么

1. 创建窗口并设置白色背景。
2. 创建 `MRFlowingLight` 组件。
3. 通过 `Transform` 指定显示区域为 `100 x 100`。
4. 调用 `initialize()` 完成内部资源和效果初始化。
5. 添加到窗口进入渲染循环。

## 相关组件介绍

### `MRFlowingLight`
- 一个流光高亮组件。
- 头文件中暴露了 `flowingLightLength`、`flowingLightColor`、`flowingLightThickness` 等效果参数。
- 很适合做 focus、hover、高亮边框和能量条扫光效果。

### `Transform`
- 负责流光区域的位置和尺寸。
- 如果后续需要适配不同分辨率，可以优先从 `Transform` 层做布局调整。

### `initialize()`
- 这类特效组件通常会在 `initialize()` 中创建内部 mesh、shader 参数或动画状态。
- 使用这类组件时，建议把 `initialize()` 视为接入流程的一部分。

## 适合继续扩展的方向

- 暴露颜色、扫光长度和厚度到外部配置。
- 支持横向、纵向和斜向多种流光方向。
- 与按钮或卡片组合，做“获得焦点时流光经过”的交互效果。

