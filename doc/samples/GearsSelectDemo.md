# GearsSelectDemo

- 对应源码：`samples/GearsSelectDemo.cpp`
- 编译目标：`GearsSelectDemo`

## Demo 用途

`GearsSelectDemo` 演示一个适合“选中态”“聚焦态”展示的点阵组件。示例本身非常简洁，因此很适合作为选择反馈控件或状态灯效的起点。

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target GearsSelectDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\GearsSelectDemo.exe"
```

## 示例做了什么

1. 创建半透明棕色背景窗口。
2. 创建 `MRGearsSelect` 组件。
3. 通过 `Transform` 设置位置和尺寸。
4. 调用 `initialize()`。
5. 启动渲染。

## 相关组件介绍

### `MRGearsSelect`
- 用于表现齿轮/点阵式的选中状态。
- 很适合做列表当前选项、仪表项激活、模块高亮等场景。

### `MRGearsSelectOptions`
- 头文件可见其核心配置较精简，主要是颜色和点大小：
  - `color`
  - `pointSize`
- 说明它更偏“状态强调”组件，而不是复杂长时动画组件。

### `Transform`
- 当前示例把尺寸设置为 `10 x 10`，更像一个小型状态点或装饰点阵。
- 如果用在真实项目中，建议结合容器尺寸做统一规范。

## 适合继续扩展的方向

- 加入 normal / selected / disabled 三种状态切换。
- 与按钮、列表项或 tab 组件集成。
- 增加淡入淡出或扫亮动画，让选中态反馈更明显。


