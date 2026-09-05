# ImageDemo

- 对应源码：`samples/ImageDemo.cpp`
- 编译目标：`ImageDemo`
- 拆分说明：批渲染统计验收 / 帧数限制 / 对象快照等调试能力已拆分至 [DebugDemo.md](DebugDemo.md)

## Demo 用途

`MRImage` 图片组件能力展示，六张卡片同屏对比：

| 卡片 | 能力 | 关键 API |
|---|---|---|
| 圆角阶梯 | shader 圆角裁剪，无需预处理贴图 | `MRImage::setRounding` |
| UV 裁剪 | 取源图子区域拉伸显示（放大镜） | `MRImage::setScissor` |
| 图片墙与合批 | 10 张图共享纹理做 alpha 呼吸动画，仍合并为 1 个 SSBO 批次 | `Material::setFloat("alpha")` + `Tween` |
| Shadow 投影 | 任意带 `MeshRenderer` 的组件叠加投影（underlay 通道） | `Shadow` 组件 |
| 图集区域 | `.basis` 图集按名取子区域显示 | `MRImage::setTexture(atlas, name)` |
| 程序化纹理 | CPU 生成的 RGBA 数据直接上屏 | `Texture::setTextureData` |

## 运行方式

```bash
cmake --build build --target ImageDemo --parallel 8
./build/ImageDemo.exe
```

## 示例做了什么

1. **圆角阶梯**：5 张 `brickwall.jpg` 并排，`setRounding` 以 14px 步进从 0 递增——
   圆角由 `image_normal` shader 在片元阶段裁剪（`geomAttr.z`），改一个参数即得任意圆角。
2. **UV 裁剪**：左图显示原图；右图 `setScissor` 取源图中心 25% 区域放大 4 倍显示，
   并叠加圆角——运行时按纹理实际尺寸计算裁剪区，不依赖图片分辨率。
   注意 `setImageUrl` 在首次渲染该纹理时才同步解码，加载完成前 `getWidth()/getHeight()`
   返回 0，直接计算 UV 会得到 NaN（画面变黑）。正确做法是订阅 `Texture::onLoaded()`
   事件，解码完成、尺寸就绪后再应用依赖尺寸的逻辑。
3. **图片墙**：10 张 `car.png` 两行排布，单个 `Tween` 按 10 个相位驱动各图
   `setFloat("alpha")` 做波浪呼吸——alpha 是 SSBO 每实例属性（`geomAttr.w`），
   动画不破坏合批，验证"动画中的同材质组件仍是一个批次"。
4. **Shadow**：`MRColor` 色块与圆角图片各挂 `Shadow` 组件
   （offset 28 / 40），阴影渐变宽度与偏移一致，展示软阴影参数。
   注意阴影走 **underlay 通道**、先于一切普通内容绘制，因此不要把带阴影的组件
   垫在不透明面板上（后画的面板会把阴影整个盖住）——本卡片直接放在窗口清屏背景上。
5. **图集区域**：`TextureAtlas` 加载 `atlas_speed.atlas`（.basis 图集），
   三个 `MRImage` 以 `setTexture(atlas, "O_*Digit_*.png")` 显示图集中的不同区域。
6. **程序化纹理**：CPU 生成 256x256 RGBA（对角渐变 + 棋盘 + 圆环），
   `Texture::setTextureData` 创建后直接显示，其中一张 `setRounding(100)` 切成圆形。

## 相关组件

### `MRImage`
- 图片组件：`setTexture(Texture)` / `setTexture(TextureAtlas, name)` 换图，
  `setRounding` shader 圆角，`setScissor` UV 裁剪；尺寸变化自动同步
  `displaySize` 到材质并触发重绘。

### `Shadow`
- 组件化投影：`addComponent<Shadow>()` + `setShadowOffset` / `setShadowColor`，
  以 underlay 批次渲染在主体之下（渐变宽度 = 偏移量）。
- underlay 先于一切普通内容绘制：阴影会被后绘制的不透明同级内容（如卡片面板）
  覆盖，带阴影的组件应直接放在窗口背景或更早绘制的内容上。

### `Texture::onLoaded`
- 文件加载完成事件（`Observable<>`）：`setImageUrl` 的图片在首次渲染该纹理时
  同步解码，事件在解码成功、`getWidth()/getHeight()` 就绪后触发，回调运行在
  触发渲染的线程。`.basis` 图集与普通图片路径都会触发；
  `setTextureData` 等同步数据路径不触发。

### `Texture::setTextureData`
- 内存纹理入口：CPU RGBA 数据（可 `std::vector` 或裸指针）直接建纹理，
  适合程序化图案、调试可视化、动态生成内容。

### 合批要点
- 同一 shader + 兼容管线状态 + 同纹理集合的组件合并为一个 SSBO 批次；
  `alpha / rounding / displaySize / model` 均为每实例数据，改它们不动批次。
- 批渲染统计与验收工具见 [DebugDemo.md](DebugDemo.md)。
