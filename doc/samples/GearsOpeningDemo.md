# GearsOpeningDemo

- 对应源码：`samples/GearsOpeningDemo.cpp`
- 编译目标：`GearsOpeningDemo`

## Demo 用途

`GearsOpeningDemo` 是齿轮开启效果的 2D 版本。它在组件本身之外额外绑定了一张背景纹理 `gearBG.png`，因此比 `GearsOpening3DDemo` 更像一个完整的界面元素，而不只是程序化点阵效果。

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target GearsOpeningDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\GearsOpeningDemo.exe"
```

## 示例做了什么

1. 创建黑色背景窗口。
2. 创建 `MRGearsOpening` 并设置位置与尺寸。
3. 加载 `../assets/textures/gearBG.png`。
4. 通过 `MeshRenderer` 和 `Material` 把纹理绑定到 `texture`。
5. 调用 `initialize()` 后渲染。

## 相关组件介绍

### `MRGearsOpening`
- 一个齿轮开启动画组件。
- 头文件里暴露了 `forward` 与 `blurRadius` 等配置，说明它适合表现方向性开合和模糊过渡。

### `Texture`
- 用于提供组件背景贴图或装饰层。
- 如果希望更接近最终 UI 视觉，这类效果组件往往会叠加一张背景图。

### `MeshRenderer` / `Material`
- 负责把背景纹理送入组件绘制通道。
- 如果后面需要换肤或主题化，通常从这里下手最直接。

### `initialize()`
- 完成内部动画数据和视觉资源的最终准备。

## 资源依赖

- `assets/textures/gearBG.png`

## 适合继续扩展的方向

- 增加可配置背景纹理和方向控制。
- 将开启动画与按钮点击或页面切换绑定。
- 与 `GearsOpening3DDemo` 一起整理为 2D / 3D 对比文档。

