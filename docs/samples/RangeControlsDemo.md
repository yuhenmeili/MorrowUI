# RangeControlsDemo

- 对应源码：`samples/RangeControlsDemo.cpp`
- 编译目标：`RangeControlsDemo`
- 合并说明：由旧 `MRSliderDemo.md`、`MRProgressBarDemo.md` 合并而来

## Demo 用途

范围类控件集中展示：`MRSlider`（滑块）与 `MRProgressBar`（进度条）的联动、
纵向方向、Tween 自动循环与反向填充。

## 运行方式

```bash
cmake --build build --target RangeControlsDemo --parallel 8
./build/RangeControlsDemo.exe
```

## 示例做了什么

1. **水平联动**：`MRSlider`（自定义 thumb 尺寸/颜色/圆角）拖动时在
   `events().onValueChanged` 回调里同步 `MRProgressBar::setProgress` 并刷新
   "当前值：N%" 标签。
2. **纵向 Slider / ProgressBar**：两者都 `setDirection(ProgressDirection::BottomToTop)`，
   Slider 拖动联动纵向进度条。
3. **Tween 自动循环**：`Tween::create(0, 1, 2s)` 线性驱动进度条，
   `onComplete` 中 `restart()` 形成循环。
4. **反向进度**：`setDirection(ProgressDirection::RightToLeft)` 固定 70%，
   演示四种填充方向之一。

## 相关组件

### `MRSlider`
- 滑块：`setValue`、`setDirection`、轨道/填充配色、`setThumbSize/setThumbColor/setThumbRounding`；
  `events().onValueChanged(MRSlider&, float)` 连续回调。

### `MRProgressBar`
- 进度条：`setProgress`、`setDirection`（LeftToRight / RightToLeft / TopToBottom /
  BottomToTop）、轨道/填充/圆角；`progress_bar` shader 支持可选的轨道/填充纹理
  （`useTrackTexture` / `useFillTexture`）。

### `Tween` / `TweenManager`
- 见 [AnimationEffectsDemo.md](AnimationEffectsDemo.md) 的补间说明；
  本 demo 用它驱动无交互的循环进度。
