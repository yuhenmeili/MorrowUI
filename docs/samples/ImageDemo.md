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
| Shadow 投影 | SDF 圆角软阴影（outer-only 裁剪，可投在卡片上） | `Shadow` 组件 |
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
4. **Shadow**：色块与圆角图片各挂 `Shadow` 组件，展示 SDF 圆角软阴影——
   默认黑色、零偏移（内区透明区正好藏在属主后面，只显示外圈柔光），
   `setShadowBlur(24/32)` 控制衰减宽度，图片阴影 `setShadowSpread(4)` 外扩，
   圆角默认跟随属主（18/20px）。阴影走普通通道、以 outer-only 裁剪绘制在
   属主之后，因此**可以直接投在卡片上**，也不会污染属主本体。
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
- SDF 圆角软阴影组件：复用属主的 MeshFilter/Transform，`shadow` shader 以
  圆角矩形 SDF 解析求值（无纹理、无 FBO）。
- 参数：`setShadowOffset`（默认零偏移柔光）、`setShadowBlur`（衰减宽度）、
  `setShadowSpread`（扩展/收缩）、`setShadowColor`（默认黑 50%）、
  `setShadowRounding`（默认跟随属主材质的 rounding）。
- shader 内做 **outer-only 裁剪**（属主轮廓内 alpha=0），阴影以普通通道
  绘制在属主之后：可投在先绘制的内容（卡片/面板）上，且不污染属主；
  偏移非零时注意内区透明区会露出其后的内容。

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
