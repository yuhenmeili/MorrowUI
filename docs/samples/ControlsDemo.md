# ControlsDemo

- 对应源码：`samples/ControlsDemo.cpp`
- 编译目标：`ControlsDemo`
- 合并说明：由旧 `ButtonDemo.md` 等单控件文档合并而来，旧文档内容已并入本文

## Demo 用途

统一展示常用交互控件：按钮类（`MRButton` / `MRTextureButton`）、选择类
（`MRCheckBox` / `MRCheckButton` / `MRToggle` / `MRRadioButton`）、菜单类
（`MROptionButton` / `MRMenuButton` / `MRPopupMenu`）、数值与布局辅助
（`MRSpinBox` / `MRSeparator` / `MRSpacer` / `HBoxContainer`），并演示事件回调、
选中状态与容器组合方式。

## 运行方式

```bash
cmake --build build --target ControlsDemo --parallel 8
./build/ControlsDemo.exe
```

## 示例做了什么

1. **Button / TextureButton 面板**：`HBoxContainer`（`spacing = 12`）内排布普通/彩色两个
   `MRButton`；下方 `MRTextureButton` 配置 Normal/Hover/Pressed/Disabled 四态纹理。
2. **Selection Controls 面板**：左侧 `MRCheckBox`（多选）、`MRCheckButton`、`MRToggle`；
   右侧三个 `MRRadioButton` 通过 `MRRadioGroup` 互斥（`group->select(radioA)` 设初值）。
   全部通过 `selectionEvents().onCheckedChanged` 回调打印状态。
3. **Menu Controls 面板**：`MROptionButton::addOption`（下拉选择）、
   `MRMenuButton::addMenuItem`（动作菜单）、独立 `MRPopupMenu`（`popupBelow` 弹出，
   含禁用项演示）。
4. **SpinBox / Separator / Spacer 面板**：两个 `MRSpinBox`（温度 16~30 步进 0.5 带后缀、
   小时 0~23）；`MRHSeparator` / `MRVSeparator` 分隔线；`HBoxContainer` +
   `MRSpacer` 演示弹性占位把"取消/应用"按钮分布到容器两端。

## 相关组件

### `MRButton`
- 文本按钮，支持文本、背景/hover/pressed 状态色、边框圆角；`events().onClicked` 提供回调。

### `MRTextureButton`
- 多态纹理按钮，`setNormalTexture/setHoverTexture/setPressedTexture/setDisabledTexture`。

### `MRCheckBox` / `MRCheckButton` / `MRToggle` / `MRRadioButton`
- 均继承可选中按钮体系（`selectionEvents().onCheckedChanged(MRSelectableButton&, bool)`）。
- `MRRadioButton` 需 `setGroup(std::shared_ptr<MRRadioGroup>)` 实现组内互斥。

### `MROptionButton` / `MRMenuButton` / `MRPopupMenu`
- 下拉选择 / 动作菜单 / 独立弹出菜单。`addOption`/`addMenuItem(id)` 回调带 id 与文本；
  `popupMenu` 通过 `attachTo(window)` 挂载，`popupBelow(aabb)` 定位弹出。

### `MRSpinBox`
- 数值输入：`setRange/setStep/setDecimals/setSuffix/setValue`，
  `events().onValueChanged(MRSpinBox&, double)`。

### `HBoxContainer` / `MRSpacer` / `MRHSeparator` / `MRVSeparator`
- 水平布局容器（`setSpacing`）+ 弹性占位 + 分隔线，构成简单的自动排布。
