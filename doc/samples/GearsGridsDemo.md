# GearsGridsDemo

- 对应源码：`samples/GearsGridsDemo.cpp`
- 编译目标：`GearsGridsDemo`

## Demo 用途

`GearsGridsDemo` 演示一个齿轮风格的点阵格栅效果组件。组件通常用于车载 UI、能量条、开机动画或特殊面板的科技感装饰。

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target GearsGridsDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\GearsGridsDemo.exe"
```

## 示例做了什么

1. 创建黑色背景窗口。
2. 创建 `MRGearsGrids` 组件。
3. 设定显示区域为 `144 x 684`，位置在 `(100, 100)`。
4. 调用 `initialize()` 初始化点阵参数和内部绘制数据。
5. 启动渲染。

## 相关组件介绍

### `MRGearsGrids`
- 一个点阵格栅类 UI 组件。
- 头文件中可见其可配置点大小、行列间距、空洞格位置等参数。
- 适合做规则阵列类科技装饰效果。

### `MRGearsGridsOptions`
- 关键参数包括：
  - `pointSize`
  - `pointRowGap`
  - `pointColumnGap`
  - `pointRow` / `pointColumn`
  - `hollowRow` / `hollowColumn`
- 通过这些参数可以控制阵列密度与镂空结构。

### `Transform`
- 负责把这个长条形点阵放到指定区域。
- 这类组件通常对宽高比敏感，建议在不同分辨率下重点观察变形情况。

## 适合继续扩展的方向

- 做一个可视化参数面板，实时调整点大小和行列数。
- 和 `GearsShine` 组合，形成基础格栅 + 高亮扫动的复合演出。
- 在 `initialize()` 后增加状态切换接口，支持多种预设样式。


