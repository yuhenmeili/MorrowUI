# Samples 文档索引

本文档与 `samples/` 下的 17 个 demo **一一对应**：每个 `samples/XxxDemo.cpp`
都有同名的 `docs/samples/XxxDemo.md`（对应源码、编译目标、用途、运行方式、
步骤与组件介绍）。编译目标名 = 源文件名（CMake 按文件名自动生成）。

| Demo | 文档 | 内容 |
|---|---|---|
| AnimationEffectsDemo | [AnimationEffectsDemo.md](AnimationEffectsDemo.md) | 动画组件全家桶：AnchorPointScale / Bounce / BrakePedal / FlowingLight / FrameAnimation + Tween |
| ControlsDemo | [ControlsDemo.md](ControlsDemo.md) | 交互控件全家桶：Button / TextureButton / CheckBox / Toggle / Radio / Menu / SpinBox / 布局辅助 |
| GLTFDemo | [GLTFDemo.md](GLTFDemo.md) | GLTF 异步加载、MR3DSceneView、IBL 光照、轨道相机 |
| GearsDemo | [GearsDemo.md](GearsDemo.md) | 齿轮特效全家桶：GearsIris / Opening / Shine / Select |
| IBLPrecomputeDemo | [IBLPrecomputeDemo.md](IBLPrecomputeDemo.md) | HDR → IBL 离线烘焙命令行工具 |
| ImageDemo | [ImageDemo.md](ImageDemo.md) | MRImage 与 SSBO 合批验证、Shadow 组件、CI 验收参数 |
| ListNavigationDemo | [ListNavigationDemo.md](ListNavigationDemo.md) | ItemList / Tree / ScrollContainer 导航控件 |
| PMemoryDemo | [PMemoryDemo.md](PMemoryDemo.md) | QNX pmem → OES 纹理零拷贝渲染（仅 QNX 生效） |
| Particles2DDemo | [Particles2DDemo.md](Particles2DDemo.md) | CPU / GPU 两种 2D 粒子系统对比 |
| RangeControlsDemo | [RangeControlsDemo.md](RangeControlsDemo.md) | Slider / ProgressBar 联动、纵向、Tween 循环、反向填充 |
| SafeDynamicVectorCanvasDemo | [SafeDynamicVectorCanvasDemo.md](SafeDynamicVectorCanvasDemo.md) | ADAS 覆盖层矢量画布（预分配顶点、逐帧重绘） |
| SafeStaticSpriteDemo | [SafeStaticSpriteDemo.md](SafeStaticSpriteDemo.md) | 安全静态图集精灵：时速表 / 档位 / 报警灯 |
| SafeStaticTextLayoutDemo | [SafeStaticTextLayoutDemo.md](SafeStaticTextLayoutDemo.md) | 安全静态文本：自建字符图集、告警词句 |
| SafeStreamTextureDemo | [SafeStreamTextureDemo.md](SafeStreamTextureDemo.md) | 安全流纹理：RVC 倒车影像模拟、多缓冲轮转 |
| SceneEffectsDemo | [SceneEffectsDemo.md](SceneEffectsDemo.md) | ParallaxBackground / CanvasModulate / Tooltip / Dialog |
| TextDemo | [TextDemo.md](TextDemo.md) | 文本输入（多行/单行/密码）、Label 对齐、富文本 |
| VideoStreamPlayerDemo | [VideoStreamPlayerDemo.md](VideoStreamPlayerDemo.md) | 视频流播放：帧源、播放控制、倍速与事件回显 |

## 历史 demo 合并映射

旧的单组件 demo 已合并为上面的整合 demo，对应关系备查：

| 旧文档（已删除） | 现文档 |
|---|---|
| AnchorPointScaleDemo.md、BounceDemo.md、BrakePedalDemo.md、FlowlightDemo.md、FrameAnimation.md | AnimationEffectsDemo.md |
| ButtonDemo.md | ControlsDemo.md |
| GearsIrisDemo.md、GearsOpeningDemo.md、GearsSelectDemo.md、GearsShineDemo.md | GearsDemo.md |
| MRSliderDemo.md、MRProgressBarDemo.md | RangeControlsDemo.md |
| AlignmentDemo.md | TextDemo.md |
| ShadowDemo.md | ImageDemo.md |
| GLTFDemo.md、IBLPrecomputeDemo.md、ImageDemo.md、PMemoryDemo.md、TextDemo.md | 同名更新 |

无旧文档、后新增的 demo：ListNavigationDemo、Particles2DDemo、Safe* 四件套、
SceneEffectsDemo、VideoStreamPlayerDemo。

## 运行方式（通用）

```bash
cmake --build build --target <DemoName> --parallel 8
./build/<DemoName>.exe
```

构建后 assets 会自动拷贝到可执行文件目录（POST_BUILD），所有 demo 默认加载
`assets/fonts/MorrowSansCN1.1-Regular.otf` 作为 `default` 字体（PMemoryDemo 除外）。
