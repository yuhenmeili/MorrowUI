# FrameAnimation

- 对应源码：`samples/FrameAnimation.cpp`
- 编译目标：`FrameAnimation`

## Demo 用途

`FrameAnimation` 演示如何使用图集资源驱动逐帧动画。示例加载 `atlas_cube.atlas` 对应的帧数据目录，把它交给 `MRFrameAnimation`，然后由组件在 `update()` 中推进当前帧。

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target FrameAnimation --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\FrameAnimation.exe"
```

## 示例做了什么

1. 创建窗口。
2. 构造 `TextureAtlas`：
   - atlas 文件：`../assets/textures/frame_animation/atlas_cube.atlas`
   - 图像目录：`../assets/textures/frame_animation/`
3. 创建 `MRFrameAnimation`。
4. 调用 `setTextureAtlas(atlas)` 注入帧数据。
5. 设置显示区域并开始渲染。

## 相关组件介绍

### `MRFrameAnimation`
- 一个基于图集的逐帧动画组件。
- 内部会维护当前帧、动画时间和 atlas 区域切换。
- 适合做表情、提示动画、loading、仪表指针序列等场景。

### `TextureAtlas`
- 把多个帧资源组织为统一图集与元数据。
- 能减少纹理切换成本，同时便于定义序列帧名称和播放顺序。

### `Animation<AtlasRegionSharedPtr>`
- 从头文件可以看出 `MRFrameAnimation` 内部使用动画对象保存 atlas 帧序列。
- 这让逐帧播放逻辑和纹理切换逻辑相对解耦。

### `Transform`
- 控制动画显示区域的位置和大小。
- 在 UI 中经常配合 atlas 动画做固定区域播放。

## 资源依赖

- `assets/textures/frame_animation/atlas_cube.atlas`
- `assets/textures/frame_animation/`

## 适合继续扩展的方向

- 增加暂停、继续、重置接口演示。
- 补一个可切换多个 atlas 的对比 demo。
- 和按钮事件联动，实现点击后播放一次的转场动画。


