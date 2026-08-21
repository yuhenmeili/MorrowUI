# MRSliderDemo

- 对应源码：`samples/MRSliderDemo.cpp`
- 编译目标：`MRSliderDemo`

## Demo 用途

演示 `MRSlider` 滑杆组件的交互：鼠标按下/拖拽调整数值、方向定义、滑块（thumb）颜色与尺寸定制，以及数值变化回调。

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\MorrowUI\cmake-build-debug-mingw" --target MRSliderDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\MorrowUI\cmake-build-debug-mingw\MRSliderDemo.exe"
```

## 示例做了什么

1. 创建一条横向滑块（默认左→右），设置轨道灰、填充蓝、圆角，滑块 32×32 蓝色圆角块，初始值 0.3；
2. 创建一条纵向滑块（下→上），填充绿、滑块绿色，初始值 0.6；
3. 两个滑块都订阅 `events().onValueChanged`，拖拽时在日志打印当前归一化值。

## 相关组件介绍

### `MRSlider`
- 继承 `MRProgressBar`，复用其渲染与全部 setter；额外挂载 `Interaction` 组件，支持按下/拖拽改值。
- `setValue`/`getValue` 是 `setProgress`/`getProgress` 的语义别名，值为 0..1 归一化。
- `setThumbSize`/`setThumbColor`/`setThumbRounding` 定制滑块；`setInteractive` 开关交互。
- `events().onValueChanged.connect(...)` 在值变化时广播，参数包含 Slider 源对象
  和归一化值。
- 返回的 Connection 必须保存到所需监听周期结束。
- 拖拽依赖平台层指针捕获：按下后即使指针移出控件，MOVE/RELEASE 仍会路由回滑块。

## 资源依赖

- 无外部资源依赖（纯颜色渲染）。

## 适合继续扩展的方向

- 滑块改纹理（把 thumb 换成 `MRImage` + 自定义贴图）。
- 增加 `step` 离散步进与 min/max 区间。
- 增加键盘方向键/滚轮调节。
