# AnimationEffectsDemo

- 对应源码：`samples/AnimationEffectsDemo.cpp`
- 编译目标：`AnimationEffectsDemo`
- 合并说明：由旧 `AnchorPointScaleDemo.md`、`BounceDemo.md`、`BrakePedalDemo.md`、`FlowlightDemo.md`、`FrameAnimation.md` 合并而来

## Demo 用途

集中展示引擎的动画能力，覆盖三种动画驱动方式：Transform/Tween 补间、Shader 参数动画、
图集帧动画。五个动画组件以卡片形式同屏展示，每个卡片带标题和说明。

## 运行方式

```bash
cmake --build build --target AnimationEffectsDemo --parallel 8
./build/AnimationEffectsDemo.exe
```

## 示例做了什么

1. 创建 `Engine`，注册默认字体，用 `MRColor` 绘制五个卡片底板与标题。
2. **MRAnchorPointScale 卡片**：设置 `Transform::setPivot(0, 1, 0)`（左下角），用
   `Tween(0.15 → 1.0, 1.5s)` 驱动 `setScale` 并循环 `restart()`，演示枢轴点缩放。
3. **MRBounce 卡片**：材质设置 `bounceTimes/scaleRange/duration/meshCenter`，用
   `Tween(0 → 1)` 驱动 `timeDelta` uniform 循环播放，演示 Shader 参数动画。
4. **MRBrakePedal 卡片**：材质绑定三张纹理（踏板/白圈/灰圈）+ `duration`，
   同样用 Tween 驱动 `timeDelta`，演示多纹理合成的制动提示动画。
5. **MRFlowingLight 卡片**：调用 `initialize()` 后沿组件边缘循环流光。
6. **MRFrameAnimation 卡片**：加载 `TextureAtlas`（`atlas_cube.atlas`），
   `setTextureAtlas` 后按图集帧循环播放。

## 相关组件

### `MRAnchorPointScale`
- 以指定 Pivot（锚点）为中心的缩放动画组件，常用于放大镜、聚焦高亮。

### `MRBounce`
- 弹跳动画组件，动画逻辑在 Shader 中实现（`bounce` shader）。
- 动画参数通过 `Material::setFloat("bounceTimes"/"scaleRange"/"duration"/"timeDelta")`
  与 `setVector("meshCenter", ...)` 传入，`timeDelta` 由外部时钟驱动。

### `MRBrakePedal`
- 制动踏板提醒动画组件，三张纹理在 Shader 中合成（`brake_pedal` shader）。

### `MRFlowingLight`
- 边缘流光组件（`flowing_light` shader），`initialize()` 后自动循环。

### `MRFrameAnimation`
- 图集帧动画组件，基于 `TextureAtlas` 的命名区域按帧轮播。

### `Tween` / `TweenManager`
- 轻量补间：`Tween::create(from, to, durationSeconds)`，链式 `setEase().onUpdate().onComplete()`，
  `restart()` 循环；实例需加入 `TweenManager::getInstance()` 才会被驱动。
