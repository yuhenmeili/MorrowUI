# BounceDemo

- 对应源码：`samples/BounceDemo.cpp`
- 编译目标：`BounceDemo`

## Demo 用途

`BounceDemo` 展示一个由 shader 参数驱动的弹跳动效。它不是直接修改 widget 的几何位置，而是通过材质参数 `bounceTimes`、`scaleRange`、`duration`、`timeDelta` 来控制视觉效果，因此很适合作为“shader 动效组件”的基础参考。

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target BounceDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\BounceDemo.exe"
```

## 示例做了什么

1. 加载纹理 `../assets/textures/bounce/aeb_r.png`。
2. 创建 `MRBounce` 组件，设置位置和尺寸。
3. 通过材质参数配置弹跳次数、缩放幅度、时长和中心点。
4. 用一个 1 秒线性 `Tween` 更新 `timeDelta`。
5. 在 `onComplete` 中调用 `restart()`，形成循环播放。

## 相关组件介绍

### `MRBounce`
- 封装了弹跳类视觉效果的 UI 组件。
- 更适合做提示动效、呼吸感强调或仪表项激活反馈。

### `Material`
- 负责动效参数输入。
- 本 demo 中的核心参数包括：
  - `bounceTimes`：弹跳次数
  - `scaleRange`：缩放范围
  - `duration`：单次动画时长
  - `timeDelta`：当前时间进度
  - `meshCenter`：几何中心

### `Tween`
- 这里主要承担时间推进器的角色。
- 相比直接使用帧时间累加，Tween 更方便做循环、缓动和统一管理。

### `Transform`
- 只负责组件的摆放和尺寸。
- 动效主体不靠 `Transform` 位移完成，而是在 shader 内部表现出来。

## 资源依赖

- 图片：`assets/textures/bounce/aeb_r.png`

## 适合继续扩展的方向

- 改成 hover 或 click 时触发，而不是开机即循环播放。
- 将 `bounceTimes` 和 `scaleRange` 暴露为外部配置，便于组件复用。
- 和 `MRButton` 结合，做按钮点击反馈动效。


