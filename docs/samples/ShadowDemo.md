# ShadowDemo

- 对应源码：`samples/ShadowDemo.cpp`
- 编译目标：`ShadowDemo`

## Demo 用途

`ShadowDemo` 用一个红色矩形图片演示 `Shadow` 组件的基础用法。它很适合作为 UI 阴影效果的最小样例，用来确认偏移、颜色和透明度是否符合预期。

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target ShadowDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\ShadowDemo.exe"
```

## 示例做了什么

1. 创建白色背景窗口。
2. 创建一个 `MRImage`，把 shader 切到 `default_color` 并设置为红色。
3. 为这个 image 添加 `Shadow` 组件。
4. 设置阴影偏移 `(5, 5)` 和半透明黑色阴影。
5. 启动渲染。

## 相关组件介绍

### `Shadow`
- 一个挂在 widget 上的阴影组件，而不是独立控件。
- 提供 `setShadowOffset()` 与 `setShadowColor()` 两个主要接口。
- 适合给图片、按钮、卡片等元素追加层次感。

### `MRImage`
- 在这里作为被投影的宿主组件。
- 由于是纯色矩形，特别适合观察阴影边缘和偏移效果。

### `Material`
- 负责把 `MRImage` 渲染成红色底板。
- 阴影组件本身则会维护自己的 shadow material。

## 适合继续扩展的方向

- 增加模糊半径、扩散大小等更丰富的阴影参数。
- 对比不同偏移方向和透明度下的视觉结果。
- 把 `Shadow` 接入 `MRButton`，形成带阴影按钮示例。

