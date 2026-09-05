# IBLPrecomputeDemo

- 对应源码：`samples/IBLPrecomputeDemo.cpp`
- 编译目标：`IBLPrecomputeDemo`

## Demo 用途

命令行 IBL 预计算工具：输入一张 HDR 环境贴图，离线烘焙出 PBR 渲染所需的三类数据
（漫反射辐照度图、镜面反射图集、BRDF LUT），供 `GLTFDemo` / `MR3DSceneView` 的
`setIBLFromDirectory` 使用。无窗口、跑完即退出。

## 启动参数

```text
IBLPrecomputeDemo [inputHDR] [outputDirectory]
```

默认参数：

- `inputHDR`：`assets/textures/hdr/symmetrical_garden_1k.hdr`
- `outputDirectory`：与输入同目录派生

## 运行方式

```bash
cmake --build build --target IBLPrecomputeDemo --parallel 8
./build/IBLPrecomputeDemo.exe
```

## 示例做了什么

1. 填充 `IBLPrecomputeOptions`：
   - 漫反射：128x64、采样 256；
   - 镜面图集：基础 512x256、5 级 mip、采样 256；
   - BRDF LUT：256、采样 512；
   - `rgbmRange = 64`，`writeHDRDebugImages = false`。
2. 调用 `IBLPrecompute::bake(options, result, error)` 一次性烘焙。
3. 打印输出路径：`irradiancePNGPath`、`specularPNGPath`（图集尺寸）、
   `brdfLUTPNGPath`。

## 相关组件

### `IBLPrecompute` / `IBLPrecomputeOptions` / `IBLPrecomputeResult`
- IBL 离线烘焙管线（对应 gltf_pbr shader 的 IBL 采样约定）。
- 输出目录整体即可作为 `MR3DSceneView::setIBLFromDirectory(dir, intensity)` 的入参，
  GLTFDemo 默认参数即指向本工具的输出。
