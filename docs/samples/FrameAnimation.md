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

## 图集生成工具与格式说明

### 推荐工具

推荐使用 **GDX Texture Packer**（libGDX 生态的纹理打包工具）来生成 `.atlas` 文件与对应的图集纹理。当然，任何能输出符合下述格式的工具也都可以使用。

### .atlas 文件格式

`.atlas` 是一个纯文本元数据文件，描述每帧在图集纹理中的位置。以 `atlas_cube.atlas` 为例：

```
atlas_cube.basis
size:2048,2048
repeat:none
256
index:11
bounds:2,1550,256,256
256
index:24
bounds:2,1292,256,256
...
```

格式规则如下：

| 行 | 说明 | 示例 |
|----|------|------|
| 第 1 行 | 图集纹理文件名，**必须以 `atlas_` 开头** | `atlas_cube.basis` |
| 第 2 行 | 纹理尺寸，格式 `size:<宽>,<高>` | `size:2048,2048` |
| 第 3 行 | 重复模式，固定为 `repeat:none` | `repeat:none` |
| 后续每两行为一组 | 一帧的定义 | — |

每帧由 **两行** 组成，支持两种模式：

**模式 A — 索引模式**（适用于统一尺寸的逐帧动画，如 `atlas_cube.atlas`）：

```
<帧尺寸>
index:<帧序号>
bounds:<left>,<top>,<width>,<height>
```

- 第一行是一个整数，表示该帧的尺寸（如 `256`，仅作标记用）
- `index` 指定帧的播放序号（帧会按 `index` 升序排列）
- `bounds` 指定该帧在图集纹理中的像素区域：`left,top,width,height`

**模式 B — 命名模式**（适用于散图打包，如 `atlas_speed.atlas`）：

```
<原始文件名>.png
bounds:<left>,<top>,<width>,<height>
```

- 第一行是原始图片文件名（如 `O_HundredDigit_1.png`）
- `bounds` 同上

> **通用规则：**
> - 文件中空格、换行均会被自动去除，对缩进无强制要求。
> - 每行以第一个冒号 `:` 分割为 key 和 value，value 中的多个字段以逗号 `,` 分隔。
> - 不含冒号的行被识别为帧名称/标识符（即上述两种模式的第一行）。

### 图集纹理

- 纹理文件需与 `.atlas` 放在同一目录。
- 支持的纹理格式包括 `.png`、`.basis` 等，取决于引擎的图像加载能力。

## 适合继续扩展的方向

- 增加暂停、继续、重置接口演示。
- 补一个可切换多个 atlas 的对比 demo。
- 和按钮事件联动，实现点击后播放一次的转场动画。


