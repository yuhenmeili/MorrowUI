# ImageDemo

- 对应源码：`samples/ImageDemo.cpp`
- 编译目标：`ImageDemo`

## Demo 用途

`ImageDemo` 展示最基础的图片组件使用方式，并顺带演示了同一张纹理在多个 `MRImage` 实例之间复用。它非常适合拿来验证图片加载、尺寸拉伸和批量摆放效果。

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target ImageDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\ImageDemo.exe"
```

启用按需渲染：

```powershell
.\build\ImageDemo.exe --request-render
```

静态场景完成首帧和资源加载触发的必要帧后，会进入事件等待；Widget、输入、resize、
异步资源或动画发出 render request 时再继续渲染。`--frames` 在该模式下统计实际
完成的渲染帧，因此没有新请求时会等待，而不是用空闲循环补足帧数。

## 示例做了什么

1. 创建窗口并设置白色背景。
2. 加载图片 `../assets/textures/img.bmp`。
3. 循环 10 次创建 `MRImage`。
4. 每个图片都设置不同的 x 坐标和略有变化的高度。
5. 通过 `MeshRenderer` 材质把同一张纹理绑定给所有图片实例。

## 相关组件介绍

### `MRImage`
- 基础图像显示组件。
- 支持直接设置 `Texture`，也支持 atlas 方式设置局部图片区域。
- 适合做图标、背景图、占位图和纯贴图 UI。

### `Texture`
- 负责加载与持有图像纹理资源。
- 当前示例使用 `ImageType::IMAGE` 和普通 BMP 文件。

### `MeshRenderer` / `Material`
- `MRImage` 内部通过 mesh + material 完成显示。
- 本 demo 显式取出材质并调用 `setTexture("texture", textAtlas)`，便于理解纹理绑定位置。

### `Transform`
- 决定每个图片的屏幕位置和尺寸。
- 示例通过循环构造不同高度，便于观察拉伸效果。

## 资源依赖

- 图片：`assets/textures/img.bmp`

## 适合继续扩展的方向

- 增加圆角、裁剪和 atlas 子区域显示示例。
- 对比不同图片格式的加载效果。
- 加入 hover 或 click，作为图片按钮 demo 的起点。

