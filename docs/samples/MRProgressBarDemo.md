# MRProgressBarDemo

- 对应源码：`samples/MRProgressBarDemo.cpp`
- 编译目标：`MRProgressBarDemo`

## Demo 用途

演示 `MRProgressBar` 进度条组件的基本用法：方向定义、轨道/填充颜色、圆角，以及用 `Tween` 驱动的循环填充动画。

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\MorrowUI\cmake-build-debug-mingw" --target MRProgressBarDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\MorrowUI\cmake-build-debug-mingw\MRProgressBarDemo.exe"
```

## 示例做了什么

1. 创建一条横向进度条（默认左→右），设置轨道灰、填充蓝，圆角 12，并用 `Tween` 在 0→1 间循环动画；
2. 创建一条横向反向进度条（右→左），填充为橙色，静态值 0.7；
3. 创建一条纵向进度条（下→上），填充为绿色，静态值 0.4。

## 相关组件介绍

### `MRProgressBar`
- 单个 quad 同时渲染轨道与填充，片元着色器按 `progress`/`direction` 裁剪填充区域，圆角复用统一 discard 算法。
- `setProgress(float)` 设置 0..1 归一化进度；`setDirection(ProgressDirection)` 设置方向（`LeftToRight`/`RightToLeft`/`BottomToTop`/`TopToBottom`）。
- `setTrackColor`/`setFillColor` 设置轨道与填充颜色；`setRounding` 设置圆角。
- 支持可替换纹理：`setTrackTexture`/`setFillTexture` 传入纹理后，颜色会作为纹理的 tint 叠加。

## 资源依赖

- 无外部资源依赖（纯颜色渲染）。

## 适合继续扩展的方向

- 给轨道/填充换自定义纹理（`setTrackTexture`/`setFillTexture`）实现皮肤化。
- 增加 min/max 数值区间与 `step` 步进。
- 增加分段着色或条纹纹理。
