# BrakePedalDemo

- 对应源码：`samples/BrakePedalDemo.cpp`
- 编译目标：`BrakePedalDemo`

## Demo 用途

`BrakePedalDemo` 演示一个刹车踏板风格的动效组件。示例同时加载踏板底图、白色圆形和灰色圆形三张纹理，再用 `timeDelta` 驱动材质参数，形成组合式视觉动画。

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target BrakePedalDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\BrakePedalDemo.exe"
```

## 示例做了什么

1. 创建窗口并设置黑色背景。
2. 分别加载：
   - `brakePedal.png`
   - `whiteCircle.png`
   - `grayCircle.png`
3. 创建 `MRBrakePedal`，设置为 `256 x 256`。
4. 把三张贴图绑定到材质参数 `brakePedalTexture`、`grayTexture`、`whiteTexture`。
5. 通过 `Tween` 持续更新 `timeDelta`。

## 相关组件介绍

### `MRBrakePedal`
- 一个面向车载/仪表风格的 UI 动效组件。
- 适合承载多层贴图合成和特定 shader 演出。

### `Texture`
- 本 demo 用多张图片提供不同图层的视觉素材。
- 当组件本身是“复合效果”时，通常会有多个 texture slot。

### `Material`
- 负责组织多张纹理与动画参数。
- `duration` 和 `timeDelta` 是控制动画节奏的关键输入。

### `TweenManager`
- 统一接管时间轴更新。
- 当前示例只播放一次；如果希望循环，可在 `onComplete` 中增加 `restart()`。

## 资源依赖

- `assets/textures/brake_pedal/brakePedal.png`
- `assets/textures/brake_pedal/whiteCircle.png`
- `assets/textures/brake_pedal/grayCircle.png`

## 适合继续扩展的方向

- 把动画进度和真实刹车力度做绑定。
- 增加按下、释放两个阶段，形成完整 pedal state demo。
- 暴露颜色和材质参数，支持不同主题皮肤。

