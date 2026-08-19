# MorrowUI 相比 Godot 2D 组件的缺口分析

## 1. 文档目的

对照 Godot 的 2D 组件体系，梳理 MorrowUI 当前缺失的通用控件，并按 HMI/车机场景的价值给出优先级，作为后续组件开发的路标。

本文不是实现任务清单；每个组件落地前仍需单独设计（shader、SSBO 布局、事件模型）。

## 2. 前提

Godot 的 2D 组件分两大类：

- **Node2D**：2D 场景节点（Sprite、Polygon、Particle、物理体等）；
- **Control**：UI 控件（Label、Button、ProgressBar、容器等）。

MorrowUI 本质是 HMI/UI 引擎，因此本文**以 Control 为主**，Node2D 只挑与 UI 绘制相关的部分；纯物理/碰撞/TileMap 等游戏向节点单独归为一类，默认不在范围内。

## 3. 结论摘要

按价值排序，建议优先补齐：

1. **环形/径向进度**（对应 Godot 的 `TextureProgressBar` 径向填充，而非不存在的 `ProgressRing` 节点）—— 仪表盘、电量环、车速环；
2. **进度条**（`ProgressBar`）—— 音量、亮度、加载；
3. **九宫格缩放**（`NinePatchRect`）—— UI 皮肤任意拉伸不变形；
4. **滑杆**（`Slider`/`Range`）—— 音量、座椅、空调调节；
5. **单行输入**（`LineEdit`）—— 目前只有多行 `MRTextEdit`。

## 4. 现有组件对照

| MorrowUI | Godot 对应 |
|---|---|---|
| MRLabel | Label |
| MRRichTextLabel | RichTextLabel |
| MRTextEdit | TextEdit（多行） |
| MRButton / MRTextureButton | Button / TextureButton |
| MRImage | TextureRect |
| MRColor | ColorRect |
| MRFrameAnimation | AnimatedSprite2D |
| HBox/VBox/Grid/Center/MarginContainer | 同名容器 |
| MR3DSceneView | SubViewport + 3D |
| Shadow / Tween / Interaction | — |

## 5. 缺失清单

表中“已完成”表示组件代码、sample 和基础行为测试已经落地；“未完成”表示仍处于缺口状态。

### 5.1 刚需级（车机 UI 基本都会用到）

| 状态 | 组件 | 说明 |
|---|---|---|
| 未完成 | 环形/径向进度 | 仪表盘、电量圈、车速环。Godot 无独立 `ProgressRing` 节点，见第 6 节 |
| 已完成 | 进度条 | 线性进度（音量、亮度、加载），对应 `MRProgressBar` |
| 未完成 | NinePatchRect（9-slice） | 圆角/描边面板任意拉伸不变形，UI 皮肤切图刚需；当前只能用 `rounding` 硬算圆角 |
| 已完成 | Slider / HSlider / VSlider | 滑杆（Range 基类），音量、座椅、空调调节，对应 `MRSlider` |
| 未完成 | LineEdit（单行输入） | 带 placeholder/密码模式/单行回车的输入框；目前只有多行 `MRTextEdit` |

### 5.2 建议级（完善交互会用到）

| 状态 | 组件 | 说明 |
|---|---|
| 已完成 | CheckBox / CheckButton、RadioButton、Toggle 按钮 | 开关、多选、单选，对应 `MRCheckBox`、`MRCheckButton`、`MRRadioButton`、`MRToggle` |
| 已完成 | OptionButton / PopupMenu / MenuButton | 下拉选择、菜单，对应 `MROptionButton`、`MRPopupMenu`、`MRMenuButton` |
| 未完成 | ScrollContainer + ScrollBar | 长列表滚动；布局容器里无 scroll |
| 未完成 | SpinBox | 数值步进（时钟、温度设定） |
| 未完成 | HSeparator / VSeparator / Spacer | 分隔线、弹性占位 |

### 5.3 可选级（2D 绘制/特效）

| 组件 | 说明 |
|---|---|
| Line2D | 折线/描边，表盘刻度线、指针外圈 |
| Path2D + PathFollow2D | 物体沿路径运动，指针沿弧线摆动 |
| GPUParticles2D / CPUParticles2D | 粒子特效（发光、飘落、氛围） |
| CanvasModulate | 全局色调 / 夜间模式一键变暗 |
| ParallaxBackground / Parallax2D | 视差背景 |
| Tooltip、Popup / PopupPanel / Window / Dialog | 弹窗、确认框、提示气泡 |
| ItemList / Tree | 列表、树形菜单 |
| VideoStreamPlayer | 视频播放（开机动画、倒车影像叠加） |

### 5.4 基本可跳过（游戏向，UI 引擎一般不需要）

物理/碰撞（`StaticBody2D / RigidBody2D / CharacterBody2D / Area2D / CollisionShape2D / RayCast2D / Joint2D`）、`TileMap / TileMapLayer`、`Camera2D`、`NavigationRegion2D`、`AudioStreamPlayer2D`。

## 6. 关于 ProgressRing 的更正

Godot 4.x **没有**名为 `ProgressRing` 的独立节点（本文初稿曾误称）。Godot 中实现环形/径向进度的常规做法是：

- **TextureProgressBar**：用纹理 + 径向填充模式（`radial_fill_degrees`、`radial_initial_angle`、`fill_mode`），环形效果靠一张环形纹理实现；
- **自定义 Control**：重写 `_draw()`，用 `draw_arc()` 直接画弧。

因此 MorrowUI 把该组件命名为 `MRProgressRing`（或 `MRRingProgress`）没有问题，但对照的 Godot 概念应是 `TextureProgressBar`，而非 `ProgressRing` 节点。

## 7. 建议落地顺序

1. 环形/径向进度 —— 可复刻 `default_color` 的 shader，加环形/扇形裁剪（SDF 或角度 + 内外半径判别）；
2. NinePatchRect —— 9 个顶点 + 中间区域拉伸；
3. 进度条 —— 复用矩形填充 + 分段着色；
4. Slider —— 进度条 + 拖拽交互；
5. LineEdit —— 复用 `MRTextEdit` 的文本渲染，限制单行。

其中环形进度与 NinePatchRect 对车机场景价值最高，实现成本也最低。
