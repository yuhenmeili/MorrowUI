# GearsOpening3DDemo

- 对应源码：`samples/GearsOpening3DDemo.cpp`
- 编译目标：`GearsOpening3DDemo`

## Demo 用途

`GearsOpening3DDemo` 展示一个带 3D 纵深感的齿轮开启动效组件。相较 `GearsOpeningDemo` 的 2D 版本，这个组件更偏程序化点阵/粒子式表现，适合用于开机动画、仪表面板切换或特殊状态进入动画。

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target GearsOpening3DDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\GearsOpening3DDemo.exe"
```

## 示例做了什么

1. 创建黑色背景窗口。
2. 创建 `MRGearsOpening3D`。
3. 将其放置到 `198 x 708` 的长条区域。
4. 调用 `initialize()` 完成内部数据初始化。
5. 打印日志并进入渲染循环。

## 相关组件介绍

### `MRGearsOpening3D`
- 一个 3D 感齿轮开启动效组件。
- 适合用在页面切换、启动页或功能开启的视觉演出中。

### `MRGearsOpening3DOptions`
- 头文件给出了多个关键参数：
  - `blurRadius`
  - `pointSize`
  - `pointColorAlphaOffset`
  - `pointRowGap` / `pointColumnGap`
  - `pointRow` / `pointColumn`
  - `hollowRow` / `hollowColumn`
- 这些信息表明组件主要由点阵结构和模糊/深度感共同构成。

### `Transform`
- 用于定义组件的显示区域。
- 对于这类条形动效，尺寸比例通常直接影响观感。

## 适合继续扩展的方向

- 增加 `forward/reverse` 两种播放方向的示例。
- 加入外部 trigger，让动画在点击或状态变化时播放。
- 与 2D 版本同屏对比，便于调试视觉差异。

