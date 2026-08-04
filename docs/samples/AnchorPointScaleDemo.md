# AnchorPointScaleDemo

- 对应源码：`samples/AnchorPointScaleDemo.cpp`
- 编译目标：`AnchorPointScaleDemo`

## Demo 用途

`AnchorPointScaleDemo` 用一张图片演示“以指定锚点为中心做缩放”的效果。示例把 `Transform` 的 pivot 设为 `(0.0, 1.0, 0.0)`，也就是以边角为锚点之一，再配合 `Tween` 持续改变 `scale`，从而直观看到缩放中心不是几何中心时的表现。

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target AnchorPointScaleDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\AnchorPointScaleDemo.exe"
```

## 示例做了什么

1. 加载 `../assets/textures/img.jpg` 作为贴图。
2. 创建 `MRAnchorPointScale` 组件，并设置位置与尺寸。
3. 通过 `MeshRenderer` 材质把贴图绑定到 `texture`。
4. 设置 `Transform::setPivot(0.0f, 1.0f, 0.0f)`。
5. 用 `Tween` 将缩放从 `0.0` 插值到 `1.0`，在 `onUpdate` 中实时调用 `setScale()`。

## 相关组件介绍

### `MRAnchorPointScale`
- 一个用于演示锚点缩放效果的 UI 组件。
- 常和 `Transform::setPivot()` 一起使用，用来验证缩放参考点是否符合预期。

### `Transform`
- 管理位置、尺寸、缩放、pivot 等变换信息。
- 这个 demo 的关键点是 pivot 先于缩放配置完成，否则视觉结果会不一致。

### `Tween` / `TweenManager`
- `Tween` 负责插值时间轴。
- `TweenManager` 负责统一管理和驱动 tween 生命周期。
- 当前示例使用 `EaseType::Linear`，因此缩放速度恒定，便于观察锚点效果。

### `MeshRenderer` / `Material`
- `MeshRenderer` 提供绘制能力。
- `Material` 用来绑定贴图、shader 参数等。
- 这里主要承担纹理显示功能。

## 资源依赖

- 图片：`assets/textures/img.jpg`

## 适合继续扩展的方向

- 把四个角和中心点的 pivot 做成多个按钮切换。
- 增加 `EaseIn/Out` 曲线，比较不同插值对缩放观感的影响。
- 把旋转、位移与缩放组合在一起，形成更完整的变换 demo。


