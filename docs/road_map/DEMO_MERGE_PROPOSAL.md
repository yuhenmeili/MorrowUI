# samples Demo 合并与精简建议

> 目的：在不明显减少组件覆盖面的前提下，降低 `samples` 目录中 Demo 的数量，避免为了演示一个很小的 API 或一个相近组件就单独维护一个可执行文件。
>
> 本文只做整理建议，当前不修改任何 `.cpp`、CMake 或资源文件，待审查确认后再实施。

## 1. 当前情况

整理开始时 `samples` 目录共有 **31 个 `.cpp` Demo**；当前实施完成后为 **17 个**。根目录 `CMakeLists.txt` 使用：

```cmake
file(GLOB DEMO_LIST ${CMAKE_CURRENT_SOURCE_DIR}/samples/*.cpp)
```

并为每个 `.cpp` 自动生成一个独立可执行目标。因此，Demo 文件数量基本等于构建目标数量，也等于需要单独维护、编译和启动验证的样例数量。

现有 Demo 大致可以分为：

1. **基础控件与布局**：文本、按钮、选择控件、菜单、滚动容器、列表/树、滑块、进度条、SpinBox 等。
2. **视觉与动画效果**：图片、阴影、CanvasModulate、流光、帧动画、AnchorPointScale、Bounce、BrakePedal、Gears 系列。
3. **复杂场景能力**：Parallax/Popup、粒子、视频流、GLTF/3D。
4. **底层或安全 HMI 能力**：批处理验证、内存纹理、Safe* 组件。

## 2. 总体建议

当前已经将 31 个 Demo 收敛为 **17 个**，处理原则分三档：

- **第一优先级：直接合并**  
  组件职责相近、代码结构相似，拆开维护收益较低。
- **第二优先级：场景化合并**  
  可以放在同一个 Showcase 中，但需要用分区、卡片或 Tab 保持每项能力清晰可见。
- **保留独立 Demo**  
  组件涉及独立渲染管线、外部资源、性能验证、异步生命周期或特殊平台路径，不建议为了减少文件数强行合并。

合并后的 Demo 建议采用“一个窗口展示一组相关能力”的方式，而不是简单地把多个 `main()` 机械拼接在一起。每组 Demo 应有：

- 顶部标题和组件列表；
- 每个组件一个明确的展示区域；
- 可交互能力尽量通过按钮、滑块或状态文字体现；
- 组件名和关键 API 直接显示在界面上；
- 对需要特殊运行参数的样例保留命令行说明。

## 3. 第一优先级：建议直接合并

### 3.1 `AlignmentDemo` 合并到 `TextDemo`

**建议结果：保留 `TextDemo.cpp`，吸收 `AlignmentDemo.cpp`。**

这是最明确的一组。`AlignmentDemo` 主要展示：

- `MRLabel`；
- `Transform` 的位置和尺寸；
- `HorizontalAlignment` / `VerticalAlignment`；
- 文本区域内的对齐效果；
- 通过 `MRImage` 或纯色区域作为对照背景。

`TextDemo` 已经展示：

- `MRLabel`；
- `MRTextEdit`；
- `MRLineEdit`；
- 密码输入；
- `MRRichTextLabel`；
- 字体、字号、颜色、自动换行；
- 文本变化和提交回调。

建议在 `TextDemo` 右侧新增“Label 对齐与排版”区域，至少同时展示：

| 对齐项 | 建议展示 |
| --- | --- |
| 水平对齐 | LEFT / CENTER / RIGHT |
| 垂直对齐 | TOP / CENTER / BOTTOM |
| 组合对齐 | 例如 CENTER + CENTER |
| 文本区域 | 同一文本在固定区域内切换对齐方式 |

`AlignmentDemo` 中的纯色图片背景可以改成简单的 `MRColor` 或保留一张背景图，不需要再单独维护一个可执行文件。

**注意：** 当前 `AlignmentDemo` 的示例文字写的是“居中对齐文本”，但实际设置的是 `LEFT + TOP`。合并时应同步修正文案，避免 Demo 本身造成误导。

---

### 3.2 `ProgressBarDemo` 与 `SliderDemo` 合并

**建议结果：新增 `RangeControlsDemo.cpp`，合并两个原 Demo。**

两者都属于“连续数值控件”，可以放在同一页面中展示：

- 水平进度条；
- 不同进度条样式或状态；
- Tween 驱动进度变化；
- 水平 Slider；
- 垂直 Slider；
- Slider 值变化回调；
- Slider 驱动 ProgressBar 的联动效果。

推荐场景：

```text
左侧：Slider 控制 ProgressBar
右侧：ProgressBar 自动播放 / Tween / 不同状态
下方：水平 Slider 与垂直 Slider
```

这样比两个 Demo 分别展示更有价值，因为能体现控件之间的组合使用，而不只是分别创建一个控件。

---

### 3.3 四个 `Gears*Demo` 合并

**建议结果：新增 `GearsDemo.cpp`，合并：**

- `GearsIrisDemo.cpp`
- `GearsOpeningDemo.cpp`
- `GearsSelectDemo.cpp`
- `GearsShineDemo.cpp`

这四个 Demo 的结构几乎一致：

- 创建 `Engine`；
- 创建一个 Gears 组件；
- 设置 `Transform`；
- 调用 `initialize()`；
- 添加到窗口；
- 进入渲染循环。

建议用四个并排或上下排列的展示卡片，分别保留：

| 组件 | 展示重点 |
| --- | --- |
| `MRGearsIris` | Iris 动画 / 遮罩效果 |
| `MRGearsOpening` | Opening 效果及背景纹理 |
| `MRGearsSelect` | Select 状态效果 |
| `MRGearsShine` | Shine 高光效果 |

这一组是最适合合并的“同系列组件”。合并后仍然能完整展示四个组件，不会损失功能覆盖。

---

### 3.4 `ImageDemo` 与 `ShadowDemo` 合并

**建议结果：将 `ShadowDemo` 的视觉展示部分并入 `ImageDemo`，但保留批处理验收参数。**

两者都以 `MRImage` 和 `Transform` / `MeshRenderer` 为核心。建议 `ImageDemo` 分成两个区域：

1. **图片与纹理区域**
   - 图片纹理；
   - 多实例渲染；
   - 不同尺寸；
   - 批处理统计。
2. **阴影区域**
   - `Shadow` 组件；
   - 阴影偏移；
   - 阴影颜色；
   - 有阴影与无阴影对照。

`ImageDemo` 当前带有 `--report-json`、`--frames`、`--request-render` 和 object snapshot 相关参数，具有测试/验收性质。合并时不要为了视觉展示删除这些参数；建议把它视为“视觉 Demo + 批处理验收入口”。

当前实施采用直接合并方案：`ShadowDemo` 已并入 `ImageDemo`，同时通过 `--report-json` 分支保护原有批处理验收统计；`CanvasModulateDemo` 则与 `ParallaxPopupDemo` 合并为 `SceneEffectsDemo`。

## 4. 第二优先级：建议按场景合并

### 4.1 控件总览：合并 `ButtonDemo`、`SelectionControlsDemo`、`MenuControlsDemo`

**建议结果：新增 `ControlsDemo.cpp`。**

这三个 Demo 都属于基础交互控件：

- `ButtonDemo`
  - `MRButton`
  - `MRTextureButton`
  - 普通按钮状态和纹理按钮状态
- `SelectionControlsDemo`
  - `MRCheckBox`
  - `MRCheckButton`
  - `MRToggle`
  - `MRRadioButton`
- `MenuControlsDemo`
  - `MROptionButton`
  - `MRMenuButton`
  - `MRPopupMenu`

推荐页面布局：

```text
按钮与纹理按钮 | CheckBox / CheckButton / Toggle
单选按钮       | OptionButton / MenuButton
               | PopupMenu
```

建议通过状态文本显示选中项、开关状态和菜单回调结果。合并后可形成一个完整的“交互控件总览”，比三个零散 Demo 更容易作为新用户入口。

**风险与控制：**

- 控件数量较多，页面可能变拥挤；
- 应避免把所有控件堆在同一坐标区域；
- 建议使用统一的 `createLabel`、`createPanel`、`createButton` 辅助函数；
- 如果后续页面超过窗口高度，可将菜单控件做成第二页或 Tab。

---

### 4.2 输入与辅助控件：将 `SpinBoxSeparatorSpacerDemo` 并入 `ControlsDemo`

`SpinBoxSeparatorSpacerDemo` 展示：

- `MRSpinBox`；
- `MRHSeparator`；
- `MRVSeparator`；
- `MRSpacer`；
- `HBoxContainer`；
- 按钮布局。

它和 `ButtonDemo` / `SelectionControlsDemo` 同属 UI 控件总览范畴，因此可以有两种方案：

#### 方案 A：全部并入 `ControlsDemo`（推荐减少数量）

适合目标是尽量减少 Demo 文件数量的情况。页面划分为：

1. 操作按钮；
2. 选择与开关；
3. 菜单；
4. 数值输入与容器布局。

#### 方案 B：保留为 `LayoutControlsDemo`

如果认为 `Separator`、`Spacer` 和 `HBoxContainer` 属于布局能力，而不是交互控件，则可将其与后面的列表/滚动容器分开保留。

当前建议采用方案 A；如果合并后页面过于复杂，再退回方案 B。

---

### 4.3 `ItemListTreeDemo` 与 `ScrollContainerDemo` 合并

**建议结果：新增 `ListNavigationDemo.cpp`。**

两者共同展示“可浏览内容”：

- `MRItemList`；
- `MRTree`；
- `MRScrollContainer`；
- 鼠标滚轮；
- 选中状态；
- 长内容布局。

推荐布局：

```text
左侧：ItemList
中间：Tree
右侧或下方：ScrollContainer 长列表
底部：当前选择 / 当前节点 / 滚动状态
```

如果窗口空间不足，可以使用 Tab：

- Tab 1：ItemList / Tree；
- Tab 2：ScrollContainer。

这组不建议只简单地删掉 `ScrollContainerDemo`，因为滚动容器本身仍需被覆盖；正确做法是把三个能力放到一个“列表与导航”场景中。

---

### 4.4 动画效果合并（已实施）

**实施结果：新增 `AnimationEffectsDemo.cpp`，已合并：**

- `AnchorPointScaleDemo.cpp`
- `BounceDemo.cpp`
- `BrakePedalDemo.cpp`
- `FlowlightDemo.cpp`
- `FrameAnimation.cpp`

这些 Demo 都是单组件、单对象、简单启动动画，单独运行时展示信息量偏少。现在已经组成一个动画效果墙：

| 分组 | 组件 |
| --- | --- |
| Transform / Tween | `MRAnchorPointScale` |
| Shader / Tween | `MRBounce`、`MRBrakePedal` |
| 光效 | `MRFlowingLight` |
| 图集帧动画 | `MRFrameAnimation` |

当前实现中每个效果使用独立卡片，卡片内显示：

- 组件名称；
- 关键参数；
- 播放 / 重播按钮；
- 当前动画状态。

**不建议**把这五个组件完全抽象成同一套配置驱动代码后再演示。它们的初始化方式和资源依赖不同，第一版只需要在同一个窗口中组织展示，不必过早做复杂框架。

## 5. 可考虑合并，但建议第二阶段再做

### 5.1 `CanvasModulateDemo` 与 `ParallaxPopupDemo`（已实施）

两者都属于“场景级视觉/交互效果”，职责不同，但已经合并为 `SceneEffectsDemo.cpp`：

- `CanvasModulate` 是全局色调调制；
- `ParallaxBackground` / `Parallax2D` 是视差背景；
- `Popup` / `Tooltip` / `Dialog` 是弹层交互。

合并后的 `SceneEffectsDemo.cpp` 按三个区域展示：

1. 全局色调；
2. 视差背景；
3. Tooltip / Dialog / Popup。

这样可以在一个场景中同时验证背景层滚动、全局色调调制以及弹层交互。

### 5.2 `Particles2DDemo`

当前已经同时展示：

- `MRCPUParticles2D`；
- `MRGPUParticles2D`。

本身已经是合并后的对照 Demo，不建议再与其他 Demo 合并。粒子系统需要独立的视觉空间和参数说明，和普通控件放在一起反而会降低可读性。

### 5.3 `VideoStreamPlayerDemo`

当前已经是一个较完整的场景，包含：

- `MRVideoStreamPlayer`；
- 内存 RGBA 帧源；
- 播放 / 暂停 / 停止；
- 前进 / 后退；
- 播放速度；
- 循环播放；
- 播放状态回调；
- 帧变化回调。

建议保留独立 Demo。它既是组件演示，也是视频帧输入和生命周期的参考实现。

## 6. 建议保持独立的 Demo

### 6.1 `GLTFDemo`

涉及：

- `MR3DSceneView`；
- 3D 场景；
- 异步 GLTF 加载；
- 3D 资源生命周期。

这是独立渲染管线，建议保持独立，避免和 2D/UI Demo 混合。

### 6.2 `IBLPrecomputeDemo`

这是 IBL 预计算/资源处理类能力，不是普通运行时控件 Demo。建议保持独立，并在文档中明确它属于“工具/预计算样例”，不要和 `GLTFDemo` 直接拼成一个窗口。

### 6.3 `PMemoryDemo`

它更接近内存纹理或平台能力验证，建议保持独立。若后续要归类，可以放入 `samples/diagnostics` 或在 README 中标注为“诊断/底层样例”。

### 6.4 `Safe*` 系列

当前包括：

- `SafeDynamicVectorCanvasDemo`
- `SafeStaticSpriteDemo`
- `SafeStaticTextLayoutDemo`
- `SafeStreamTextureDemo`

它们都面向安全 HMI，但底层能力差异明显：

| Demo | 主要能力 |
| --- | --- |
| `SafeDynamicVectorCanvasDemo` | 动态矢量画布、ADAS Overlay |
| `SafeStaticSpriteDemo` | 静态图集 Sprite、安全图片组件 |
| `SafeStaticTextLayoutDemo` | 静态字体图集排版 |
| `SafeStreamTextureDemo` | OES/EGLImage 或流式纹理更新 |

建议暂时保留独立，原因是这些文件同时包含较多平台、缓冲区、图集注册和逐帧更新代码，强行合并会导致：

- 单个 Demo 过大；
- 资源初始化和生命周期互相影响；
- 平台分支更难验证；
- 性能/安全问题定位变困难。

如果后续确实需要减少数量，建议只做两组：

1. `SafeStaticHmiDemo`
   - 合并 `SafeStaticSpriteDemo`
   - 合并 `SafeStaticTextLayoutDemo`
2. `SafeRuntimeOverlayDemo`
   - 合并 `SafeDynamicVectorCanvasDemo`
   - 合并 `SafeStreamTextureDemo`

不建议四个全部合并成一个超大 Demo。

## 7. 推荐的最终目录形态

以下是建议的第一阶段目标，名称仅供讨论：

| 建议文件 | 来源 | 处理 |
| --- | --- | --- |
| `TextDemo.cpp` | `TextDemo` + `AlignmentDemo` | 直接合并 |
| `ControlsDemo.cpp` | `Button` + `SelectionControls` + `MenuControls` + `SpinBoxSeparatorSpacer` | 场景合并 |
| `ListNavigationDemo.cpp` | `ItemListTree` + `ScrollContainer` | 场景合并 |
| `RangeControlsDemo.cpp` | `ProgressBar` + `Slider` | 直接合并 |
| `ImageDemo.cpp` | `Image` + `Shadow` | 直接合并，保留验收参数 |
| `AnimationEffectsDemo.cpp` | `AnchorPointScale` + `Bounce` + `BrakePedal` + `Flowlight` + `FrameAnimation` | 场景合并，已实施 |
| `GearsDemo.cpp` | 四个 `Gears*` Demo | 同系列合并 |
| `SceneEffectsDemo.cpp` | `CanvasModulate` + `ParallaxPopup` | 场景合并，已实施 |
| `Particles2DDemo.cpp` | 当前 Demo | 保留 |
| `VideoStreamPlayerDemo.cpp` | 当前 Demo | 保留 |
| `GLTFDemo.cpp` | 当前 Demo | 保留 |
| `IBLPrecomputeDemo.cpp` | 当前 Demo | 保留 |
| `PMemoryDemo.cpp` | 当前 Demo | 保留 |
| `SafeDynamicVectorCanvasDemo.cpp` | 当前 Demo | 保留 |
| `SafeStaticSpriteDemo.cpp` | 当前 Demo | 保留 |
| `SafeStaticTextLayoutDemo.cpp` | 当前 Demo | 保留 |
| `SafeStreamTextureDemo.cpp` | 当前 Demo | 保留 |

按当前实施结果，Demo 数量已经从 **31 个减少到 17 个**，同时仍覆盖当前主要组件和底层能力。

## 8. 组件覆盖检查清单

实施合并时建议逐项确认以下能力没有因删 Demo 而丢失：

### 基础渲染与节点

- [ ] `MRImage`
- [ ] `MRColor`
- [ ] `MRLabel`
- [ ] `Transform`
- [ ] `MeshRenderer`
- [ ] 纹理材质设置
- [ ] 阴影
- [ ] Canvas 全局调制

### 文本

- [ ] `MRLabel`
- [ ] 水平/垂直对齐
- [ ] `MRTextEdit`
- [ ] `MRLineEdit`
- [ ] 密码模式
- [ ] `MRRichTextLabel`
- [ ] 自动换行
- [ ] 文本变化回调
- [ ] 提交回调

### 交互控件

- [ ] `MRButton`
- [ ] `MRTextureButton`
- [ ] `MRCheckBox`
- [ ] `MRCheckButton`
- [ ] `MRToggle`
- [ ] `MRRadioButton`
- [ ] `MROptionButton`
- [ ] `MRMenuButton`
- [ ] `MRPopupMenu`
- [ ] `MRSpinBox`
- [ ] `MRSlider`
- [ ] `MRProgressBar`

### 布局与内容浏览

- [ ] `HBoxContainer`
- [ ] `MRSeparator`
- [ ] `MRSpacer`
- [ ] `MRScrollContainer`
- [ ] `MRItemList`
- [ ] `MRTree`

### 动画与视觉效果

- [ ] `MRAnchorPointScale`
- [ ] `MRBounce`
- [ ] `MRBrakePedal`
- [ ] `MRFlowingLight`
- [ ] `MRFrameAnimation`
- [ ] `MRGearsIris`
- [ ] `MRGearsOpening`
- [ ] `MRGearsSelect`
- [ ] `MRGearsShine`
- [ ] `MRParticles2D` CPU/GPU
- [ ] `MRParallaxBackground`
- [ ] `MRParallax2D`
- [ ] `MRPopup` / `MRTooltip` / `MRDialog`

### 复杂与底层能力

- [ ] `MRVideoStreamPlayer`
- [ ] `MR3DSceneView`
- [ ] GLTF 异步加载
- [ ] IBL 预计算
- [ ] 内存/流式纹理
- [ ] Safe 静态 Sprite
- [ ] Safe 静态文本
- [ ] Safe 动态矢量画布
- [ ] Safe 流媒体纹理
- [ ] Image 批处理统计和验收参数

## 9. 推荐实施顺序

实际已经按以下顺序完成主要合并：

1. **`AlignmentDemo` → `TextDemo`**：已完成。
2. **`ProgressBarDemo` + `SliderDemo`**：已完成。
3. **四个 `Gears*Demo`**：已完成。
4. **`ImageDemo` + `ShadowDemo`**：已完成，并保留命令行验收路径。
5. **控件总览和列表导航场景**：已完成。
6. **动画效果墙和场景效果合并**：已完成 `AnimationEffectsDemo.cpp` 和 `SceneEffectsDemo.cpp`。

## 10. 结论

已完成的合并关系包括：

1. `AlignmentDemo` → `TextDemo`；
2. `ProgressBarDemo` + `SliderDemo` → `RangeControlsDemo`；
3. 四个 `Gears*Demo` → `GearsDemo`；
4. `ImageDemo` + `ShadowDemo` → `ImageDemo`；
5. `ButtonDemo` + `SelectionControlsDemo` + `MenuControlsDemo` + `SpinBoxSeparatorSpacerDemo` → `ControlsDemo`；
6. `ItemListTreeDemo` + `ScrollContainerDemo` → `ListNavigationDemo`；
7. `AnchorPointScaleDemo` + `BounceDemo` + `BrakePedalDemo` + `FlowlightDemo` + `FrameAnimation` → `AnimationEffectsDemo`；
8. `CanvasModulateDemo` + `ParallaxPopupDemo` → `SceneEffectsDemo`。

建议暂时不要合并 `GLTFDemo`、`IBLPrecomputeDemo`、`PMemoryDemo`、`VideoStreamPlayerDemo` 以及四个 `Safe*Demo`。它们的功能边界或验证目的比较独立，保留独立入口更利于使用和排查问题。
