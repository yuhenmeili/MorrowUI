# Samples 文档索引

本文档目录为 `samples/` 下每个 demo 提供一份使用介绍与相关组件说明，便于快速查找示例入口、运行方式和组件定位。

## 目录约定

- 源码目录：`samples/`
- 文档目录：`docs/samples/`
- 命名规则：`samples/<DemoName>.cpp` 对应 `docs/samples/<DemoName>.md`

## Demo 一览

| Demo | 说明 | 主要组件 | 文档 |
| --- | --- | --- | --- |
| `AlignmentDemo` | 文本对齐与文字区域布局 | `MRLabel`、`MRImage`、`Transform` | [AlignmentDemo.md](./AlignmentDemo.md) |
| `AnchorPointScaleDemo` | 围绕锚点做缩放动画 | `MRAnchorPointScale`、`Transform`、`Tween` | [AnchorPointScaleDemo.md](./AnchorPointScaleDemo.md) |
| `BounceDemo` | 基于 shader 参数驱动的弹跳效果 | `MRBounce`、`Tween`、`MeshRenderer` | [BounceDemo.md](./BounceDemo.md) |
| `BrakePedalDemo` | 刹车踏板动效组件演示 | `MRBrakePedal`、`Tween`、`Texture` | [BrakePedalDemo.md](./BrakePedalDemo.md) |
| `ButtonDemo` | 按钮与布局容器组合示例 | `MRButton`、`HBoxContainer`、`BaseButton` | [ButtonDemo.md](./ButtonDemo.md) |
| `FlowlightDemo` | 流光扫过效果组件 | `MRFlowingLight`、`Transform` | [FlowlightDemo.md](./FlowlightDemo.md) |
| `FrameAnimation` | 图集逐帧动画 | `MRFrameAnimation`、`TextureAtlas` | [FrameAnimation.md](./FrameAnimation.md) |
| `GearsGridsDemo` | 齿轮格栅点阵效果 | `MRGearsGrids` | [GearsGridsDemo.md](./GearsGridsDemo.md) |
| `GearsIrisDemo` | 光圈/栅格类动画效果 | `MRGearsIris` | [GearsIrisDemo.md](./GearsIrisDemo.md) |
| `GearsOpening3DDemo` | 3D 感的齿轮开启动效 | `MRGearsOpening3D` | [GearsOpening3DDemo.md](./GearsOpening3DDemo.md) |
| `GearsOpeningDemo` | 2D 齿轮开启与背景贴图 | `MRGearsOpening`、`Texture` | [GearsOpeningDemo.md](./GearsOpeningDemo.md) |
| `GearsSelectDemo` | 选中态点阵/齿轮高亮组件 | `MRGearsSelect` | [GearsSelectDemo.md](./GearsSelectDemo.md) |
| `GearsShineDemo` | 充能/扫光类齿轮点阵效果 | `MRGearsShine` | [GearsShineDemo.md](./GearsShineDemo.md) |
| `GLTFDemo` | GLTF/GLB 3D 模型加载与交互 | `MR3DSceneView`、`Scene3DAsyncLoader`、`OrbitController` | [GLTFDemo.md](./GLTFDemo.md) |
| `IBLPrecomputeDemo` | IBL 贴图预计算工具示例 | `IBLPrecompute` | [IBLPrecomputeDemo.md](./IBLPrecomputeDemo.md) |
| `ImageDemo` | 基础图片组件和批量摆放 | `MRImage`、`Texture` | [ImageDemo.md](./ImageDemo.md) |
| `PMemoryDemo` | QNX 下 PMEM/OES 纹理接入 | `MRImage`、`Texture`、`global_tools::loadPmemData` | [PMemoryDemo.md](./PMemoryDemo.md) |
| `ShadowDemo` | UI 阴影组件基础用法 | `Shadow`、`MRImage` | [ShadowDemo.md](./ShadowDemo.md) |
| `TextDemo` | 字体加载与文本渲染 | `MRLabel`、`FontManager` | [TextDemo.md](./TextDemo.md) |

## 通用运行方式

假设已在工程根目录完成 CMake 配置，可按目标名单独编译并运行某个 demo：

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target <DemoName> --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\<DemoName>.exe"
```

以 `GLTFDemo` 为例：

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target GLTFDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\GLTFDemo.exe"
```

## 适合怎么查

- 想找基础 UI：先看 `ImageDemo`、`TextDemo`、`ButtonDemo`
- 想找时间轴/动效：先看 `AnchorPointScaleDemo`、`BounceDemo`、`FrameAnimation`
- 想找仪表/点阵特效：看 `Gears*` 系列 demo
- 想找 3D 能力：看 `GLTFDemo` 与 `IBLPrecomputeDemo`
- 想找平台特性：看 `PMemoryDemo`


