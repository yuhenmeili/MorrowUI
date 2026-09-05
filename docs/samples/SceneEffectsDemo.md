# SceneEffectsDemo

- 对应源码：`samples/SceneEffectsDemo.cpp`
- 编译目标：`SceneEffectsDemo`

## Demo 用途

场景级效果组件集中展示：`MRParallaxBackground` / `MRParallax2D`（多层视差背景）、
`MRCanvasModulate`（全局色调调制/夜间模式）、`MRTooltip`（跟随定位的提示气泡）、
`MRDialog`（可复用确认弹窗）。

## 运行方式

```bash
cmake --build build --target SceneEffectsDemo --parallel 8
./build/SceneEffectsDemo.exe
```

## 示例做了什么

1. **视差背景**：`MRParallaxBackground` 内 `addLayer` 三个 `MRParallax2D` 层
   （滚动倍率 0.12 / 0.35 / 0.72，各层用 `MRColor` 色块填充）；"向左/向右滚动"按钮
   调用 `setScrollOffset` 移动背景，验证不同倍率层的位移差。
2. **CanvasModulate**：三张彩色卡片展示全局调制影响范围；
   `MRCanvasModulate` 设 `setModulateColor`（偏蓝）+ `setStrength(0)` 初始无效果，
   "切换夜间模式"按钮在 `setNightMode` 间切换。
3. **Tooltip**：`MRTooltip::create` + `setText` + `attachTo(window)`，
   按钮点击 `showFor(button->getScreenSpaceAABB())` 在目标区域旁弹出。
4. **Dialog**：`MRDialog::create` 设标题/消息，`attachTo` 后按钮触发
   `popup(380, 210)`；`dialogEvents().onConfirmed / onCanceled` 回调打印结果。

## 相关组件

### `MRParallaxBackground` / `MRParallax2D`
- 多层视差背景容器：`addLayer(layer, scaleX, scaleY)` 注册不同滚动倍率的层，
  `setScrollOffset` 统一驱动，层内内容自行布局。

### `MRCanvasModulate`
- 全局色调调制（`canvas_modulate` shader）：同时影响背景、色块、文字与按钮，
  内置夜间模式，`setStrength` 控制效果强度。

### `MRTooltip` / `MRDialog`
- 气泡提示与模态弹窗：`attachTo(window)` 挂载到根，`showFor(aabb)` / `popup(w, h)`
  定位显示；Dialog 通过 `dialogEvents()` 提供确认/取消事件。
