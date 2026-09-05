# SafeDynamicVectorCanvasDemo

- 对应源码：`samples/SafeDynamicVectorCanvasDemo.cpp`
- 编译目标：`SafeDynamicVectorCanvasDemo`

## Demo 用途

安全动态矢量画布（`SafeDynamicVectorCanvas`）演示，模拟 ADAS 智驾覆盖层：
白色虚线车道线（滚动动画）、道路边缘实线、红色/橙色/黄色动态障碍物检测框、
雷达探测连线、绿色倒车轨迹曲线、HUD 速度弧线与十字准星，全部内容逐帧重绘。

## 运行方式

```bash
cmake --build build --target SafeDynamicVectorCanvasDemo --parallel 8
./build/SafeDynamicVectorCanvasDemo.exe
```

## 示例做了什么

1. 单线程 `Engine`，深色夜间驾驶背景。
2. `SafeDynamicVectorCanvas::create(8192)`：预分配 8192 顶点的矢量画布，
   全窗口尺寸，避免运行期顶点缓冲增长。
3. 在 `engine->events().onFrameBegin` 中逐帧绘制：
   - `clear()` 清空后按窗口等比缩放系数重排全部图元；
   - `drawLine` 车道虚线（相位随帧号滚动）、道路边缘、雷达连线、速度弧线、准星；
   - `drawRect` 障碍物检测框（位置随正弦动态移动）；
   - `drawPolyline` 倒车轨迹曲线（40 段）；
   - `drawCircle` HUD 外圈；
   - 最后 `commit()` 一次性提交 GPU。
4. 全部坐标为画布局部坐标（中心原点），由 `Transform` 负责屏幕定位。

## 相关组件

### `SafeDynamicVectorCanvas`
- 面向安全场景（预分配、无运行期动态分配）的立即模式矢量画布：
  `clear → drawLine/drawRect/drawCircle/drawPolyline → commit` 的帧内流程。
- 适合 ADAS 覆盖层、雷达图、轨迹线等每帧全量重绘的动态图形。
