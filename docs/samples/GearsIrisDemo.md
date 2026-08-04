# GearsIrisDemo

- 对应源码：`samples/GearsIrisDemo.cpp`
- 编译目标：`GearsIrisDemo`

## Demo 用途

`GearsIrisDemo` 展示一个带有光圈/栅格风格的组件。根据头文件暴露的参数，这个组件同时包含圆形光圈层和网格层，因此很适合表现摄像头 iris、仪表开机焦点或 HUD 聚焦效果。

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target GearsIrisDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\GearsIrisDemo.exe"
```

## 示例做了什么

1. 创建黑色背景窗口。
2. 创建 `MRGearsIris`。
3. 把组件放在 `(100, 0)`，尺寸为 `144 x 960`。
4. 调用 `initialize()`。
5. 启动渲染。

## 相关组件介绍

### `MRGearsIris`
- 一个围绕 iris / 聚焦感设计的 UI 组件。
- 比纯点阵组件更适合表现“收束”“锁定”“聚焦”这类状态。

### `MRGearsIrisOptions`
- 从头文件可以看到主要参数包括：
  - `circleRadius`
  - `circleDelay`
  - `circleColor`
  - `gridGap`
  - `gridResetTime`
  - `gridHalfWidthParameter`
  - `gridColor`
- 这说明组件内部是多层元素叠加，而不是简单单层贴图。

### `initialize()`
- 负责完成内部网格/圆形效果初始化。
- 对于这类 shader/程序化绘制组件，建议始终在添加到窗口前完成初始化。

## 适合继续扩展的方向

- 提供 focus in / focus out 两套预设参数。
- 与按钮、选择框或 3D 视图交互联动，作为选中反馈。
- 抽出颜色和半径配置，支持深色与浅色主题。

