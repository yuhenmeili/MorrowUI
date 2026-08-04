# IBLPrecomputeDemo

- 对应源码：`samples/IBLPrecomputeDemo.cpp`
- 编译目标：`IBLPrecomputeDemo`

## Demo 用途

`IBLPrecomputeDemo` 不是 UI 场景 demo，而是一个资源预处理工具示例。它会读取 HDR 环境贴图，烘焙出：

- irradiance 贴图
- specular atlas
- BRDF LUT

这正是 `MR3DSceneView::setIBLFromDirectory()` 等 3D PBR 功能依赖的上游资源准备步骤。

## 启动参数

```text
IBLPrecomputeDemo [inputHDR] [outputDirectory]
```

默认参数：

- `inputHDR`：`../assets/textures/hdr/symmetrical_garden_1k.hdr`
- `outputDirectory`：留空时由实现自行决定输出位置

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target IBLPrecomputeDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\IBLPrecomputeDemo.exe"
```

指定 HDR 输入和输出目录：

```powershell
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\IBLPrecomputeDemo.exe" "assets/textures/hdr/symmetrical_garden_1k.hdr" "assets/textures/ibl/generated"
```

## 示例做了什么

1. 构造 `IBLPrecomputeOptions`。
2. 配置 irradiance、specular、BRDF LUT 的尺寸与采样数。
3. 调用 `IBLPrecompute::bake()` 执行烘焙。
4. 失败时输出错误信息；成功时打印生成文件路径和 atlas 尺寸。

## 相关组件介绍

### `IBLPrecompute`
- 一个环境光照预计算工具类。
- 负责从单张 HDR 环境图生成运行时 IBL 需要的多类贴图资源。
- 属于离线/预处理型能力，而不是 UI 控件。

### `IBLPrecomputeOptions`
- 定义输入文件和烘焙精度。
- 典型参数包括：
  - `irradianceWidth` / `irradianceHeight`
  - `specularBaseWidth` / `specularBaseHeight`
  - `specularMipCount`
  - `brdfLUTSize`
  - `rgbmRange`
- 调大采样数能提升质量，但会增加烘焙耗时。

### `IBLPrecomputeResult`
- 返回输出目录、结果文件路径以及 specular atlas 尺寸与 mip 信息。
- 适合被后续资源流水线或运行时加载器直接消费。

## 资源依赖

- HDR 输入：`assets/textures/hdr/symmetrical_garden_1k.hdr`

## 适合继续扩展的方向

- 增加批量 HDR 烘焙脚本化入口。
- 把输出结果整理成 `MR3DSceneView::setIBLFromDirectory()` 约定目录格式的说明文档。
- 追加性能对比：不同 sample count 对耗时与质量的影响。

