# Samples 文档索引

本文档目录为 `samples/` 下每个 demo 提供一份使用介绍与相关组件说明，便于快速查找示例入口、运行方式和组件定位。

## 目录约定

- 源码目录：`samples/`
- 文档目录：`docs/samples/`
- 命名规则：`samples/<DemoName>.cpp` 一一对应 `docs/samples/<DemoName>.md`（编译目标名 = 文件名）
- 各文档的新旧合并映射与详细索引见 [samples/README.md](samples/README.md)

## Demo 一览

| Demo | 说明                            | 主要组件 | 文档 |
| --- |-------------------------------| --- | --- |
| `AnimationEffectsDemo` | 动画组件全家桶：锚点缩放、弹跳、制动踏板、流光、图集帧动画 | `MRAnchorPointScale`、`MRBounce`、`MRBrakePedal`、`MRFlowingLight`、`MRFrameAnimation`、`Tween` | [AnimationEffectsDemo.md](samples/AnimationEffectsDemo.md) |
| `ControlsDemo` | 交互控件全家桶：按钮、选择、菜单、数值输入与布局辅助    | `MRButton`、`MRTextureButton`、`MRCheckBox`、`MRToggle`、`MRRadioButton`、`MRSpinBox` | [ControlsDemo.md](samples/ControlsDemo.md) |
| `GLTFDemo` | GLTF/GLB 3D 模型加载与交互           | `MR3DSceneView`、`Scene3DAsyncLoader`、`OrbitController` | [GLTFDemo.md](samples/GLTFDemo.md) |
| `GearsDemo` | 换挡特效全家桶：光圈、展开、高光、选中点          | `MRGearsIris`、`MRGearsOpening`、`MRGearsShine`、`MRGearsSelect` | [GearsDemo.md](samples/GearsDemo.md) |
| `IBLPrecomputeDemo` | IBL 贴图预计算工具（命令行，无窗口）          | `IBLPrecompute` | [IBLPrecomputeDemo.md](samples/IBLPrecomputeDemo.md) |
| `ImageDemo` | 图片组件与 SSBO 合批验证、阴影、CI 验收参数    | `MRImage`、`Shadow`、`batchStatistics` | [ImageDemo.md](samples/ImageDemo.md) |
| `ListNavigationDemo` | 列表选择、树形展开与长列表滚动               | `MRItemList`、`MRTree`、`MRScrollContainer` | [ListNavigationDemo.md](samples/ListNavigationDemo.md) |
| `PMemoryDemo` | QNX 下 PMEM/OES 纹理零拷贝接入        | `Texture(OES)`、`MRImage`、`global_tools::loadPmemData` | [PMemoryDemo.md](samples/PMemoryDemo.md) |
| `Particles2DDemo` | CPU / GPU 两种 2D 粒子系统对比        | `MRCPUParticles2D`、`MRGPUParticles2D` | [Particles2DDemo.md](samples/Particles2DDemo.md) |
| `RangeControlsDemo` | 滑杆与进度条：联动、纵向、Tween 循环、反向填充    | `MRSlider`、`MRProgressBar`、`Tween` | [RangeControlsDemo.md](samples/RangeControlsDemo.md) |
| `SafeDynamicVectorCanvasDemo` | ADAS 覆盖层矢量画布（预分配顶点、逐帧重绘）      | `SafeDynamicVectorCanvas` | [SafeDynamicVectorCanvasDemo.md](samples/SafeDynamicVectorCanvasDemo.md) |
| `SafeStaticSpriteDemo` | 安全静态图集精灵：时速表、档位、报警灯           | `SafeStaticSprite`、`StaticAtlasManager` | [SafeStaticSpriteDemo.md](samples/SafeStaticSpriteDemo.md) |
| `SafeStaticTextLayoutDemo` | 安全静态文本：自建字符图集与告警词句            | `SafeStaticTextLayout`、`StaticAtlasManager` | [SafeStaticTextLayoutDemo.md](samples/SafeStaticTextLayoutDemo.md) |
| `SafeStreamTextureDemo` | 安全流纹理：倒车影像模拟、多缓冲轮转            | `SafeStreamTexture` | [SafeStreamTextureDemo.md](samples/SafeStreamTextureDemo.md) |
| `SceneEffectsDemo` | 视差背景、全局色调调制、Tooltip、Dialog    | `MRParallaxBackground`、`MRCanvasModulate`、`MRTooltip`、`MRDialog` | [SceneEffectsDemo.md](samples/SceneEffectsDemo.md) |
| `TextDemo` | 文本输入（多行/单行/密码）、Label 对齐、富文本   | `MRTextEdit`、`MRLineEdit`、`MRLabel`、`MRRichTextLabel` | [TextDemo.md](samples/TextDemo.md) |
| `VideoStreamPlayerDemo` | 视频流播放：内存帧源、播放控制、倍速、事件回显       | `MRVideoStreamPlayer` | [VideoStreamPlayerDemo.md](samples/VideoStreamPlayerDemo.md) |

## 通用运行方式

在工程根目录完成 CMake 配置后，按目标名单独编译并运行某个 demo：

```bash
cmake --build build --target <DemoName> --parallel 8
./build/<DemoName>.exe
```

构建后 assets 会自动拷贝到可执行文件目录（POST_BUILD），demo 默认使用
`assets/fonts/MorrowSansCN1.1-Regular.otf`（`PMemoryDemo` 除外）。

## 适合怎么查

- 想找基础 UI：先看 `ImageDemo`、`TextDemo`、`ControlsDemo`
- 想找时间轴/动效：先看 `AnimationEffectsDemo`
- 想找仪表/点阵特效：看 `GearsDemo`
- 想找列表/导航：看 `ListNavigationDemo`
- 想找 3D 能力：看 `GLTFDemo` 与 `IBLPrecomputeDemo`
- 想找视频/流媒体：看 `VideoStreamPlayerDemo`、`SafeStreamTextureDemo`、`PMemoryDemo`
- 想找功能安全（Safe）组件：看 `Safe*` 四件套
- 想找平台特性：看 `PMemoryDemo`
