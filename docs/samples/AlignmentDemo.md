# AlignmentDemo

- 对应源码：`samples/AlignmentDemo.cpp`
- 编译目标：`AlignmentDemo`

## Demo 用途

`AlignmentDemo` 用一个黄色矩形背景配合一段文本，演示 `MRLabel` 在固定文本区域内的对齐方式。当前示例把文本放到 `512 x 512` 的区域中，并设置为左上对齐，因此非常适合拿来验证文字布局、字体资源和容器坐标是否正确。

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target AlignmentDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\AlignmentDemo.exe"
```

## 示例做了什么

1. 创建 `Engine` 和默认窗口。
2. 注册字体 `../assets/fonts/MorrowSansCN1.1-Regular.otf`，名字为 `default`。
3. 创建一个 `MRImage`，用纯色材质画出对齐参考区域。
4. 创建 `MRLabel`，设置文本、颜色和对齐方式。
5. 将图片与文本都挂到窗口，进入渲染循环。

## 相关组件介绍

### `MRLabel`
- 用于文本渲染的基础 UI 组件。
- 支持 `setText()`、`setFontColor()`、`setAlign()`。
- 适合验证字体加载、排版和对齐行为。

### `MRImage`
- 一个通用图片/矩形显示组件。
- 本 demo 中没有用贴图，而是直接通过 `MeshRenderer` 材质切到 `default_color` shader，并设置纯色 `Vector4`。
- 常用于做背景板、占位区域和图像展示。

### `Transform`
- 负责位置、尺寸等几何属性。
- 文本和背景都通过 `Transform` 设置到同一块区域，因此很容易观察对齐结果。

## 资源依赖

- 字体：`assets/fonts/MorrowSansCN1.1-Regular.otf`

## 适合继续扩展的方向

- 把 `HorizontalAlignment::LEFT / CENTER / RIGHT` 和 `VerticalAlignment::TOP / CENTER / BOTTOM` 全部做成可切换组合。
- 开启自动换行、字符间距和行距，补成完整的排版调试 demo。
- 增加多个不同字号的标签，验证字体测量和基线表现。


