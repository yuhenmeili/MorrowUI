# GearsShineDemo

- 对应源码：`samples/GearsShineDemo.cpp`
- 编译目标：`GearsShineDemo`

## Demo 用途

`GearsShineDemo` 展示一个齿轮点阵上的高亮扫光/充能效果。它非常适合做进度推进、能量蓄积、状态激活等视觉表现。

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target GearsShineDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\GearsShineDemo.exe"
```

## 示例做了什么

1. 创建黑色背景窗口。
2. 创建 `MRGearsShine` 组件。
3. 设置组件位置和尺寸为 `144 x 684` 的长条区域。
4. 调用 `initialize()`。
5. 进入渲染循环。

## 相关组件介绍

### `MRGearsShine`
- 一个在点阵基础上追加高亮移动效果的 UI 组件。
- 常见于“充能”“点亮”“扫过”这类视觉语言。

### `MRGearsShineOptions`
- 关键参数包括：
  - `direct`
  - `radiusHighlight`
  - `pointSize` / `pointSizeHighlight`
  - `pointColorAlphaOffset`
  - `pointColorHighlight`
  - 点阵行列与镂空配置
- 说明组件本身支持“基础点阵 + 高亮区域”两层变化。

### `Transform`
- 负责把效果限制在一段固定长条区域里。
- 这类组件通常适合安装在侧边装饰或仪表纵向导光槽位置。

## 适合继续扩展的方向

- 暴露方向参数，做上行/下行/中心扩散三种版本。
- 支持外部进度输入，让高亮位置与真实数值联动。

