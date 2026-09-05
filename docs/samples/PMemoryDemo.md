# PMemoryDemo

- 对应源码：`samples/PMemoryDemo.cpp`
- 编译目标：`PMemoryDemo`

## Demo 用途

演示 QNX 车载场景下的物理连续内存（pmem）缓冲区接入：把 pmem 显示缓冲直接包装成
OES 纹理并交给 `MRImage` 渲染，零拷贝对接系统图层/摄像头输出。**仅在 QNX 平台有实际
逻辑**，其他平台 `main` 直接返回（代码位于 `#ifdef QNX` 内）。

## 运行方式

```bash
# QNX 目标机上
cmake --build build --target PMemoryDemo
./build/PMemoryDemo
```

## 示例做了什么

1. 创建 `Engine`，白色清屏。
2. `global_tools::loadPmemData(pmem_hdl, "assets/textures/d_p.rgb")` 加载 pmem 数据句柄。
3. `Texture::create(ImageType::OES)` 创建外部 OES 纹理，
   `setOESTextureData(pmem_hdl, 48, 48, PixelDataFormat::RGB, 0)` 绑定 pmem 缓冲。
4. `MRImage` 持有该纹理，48x48 摆放在 `(100, 100)`。

## 相关组件

### OES 纹理（`Texture::create(ImageType::OES)`）
- 外部纹理入口：pmem / EGLImage / 平台视频缓冲统一走 OES 路径
  （`image_oes` shader 采样 `samplerExternalOES`）。
- 相关设计见 [OES_TEXTURE_FENCE_DESIGN.md](../road_map/OES_TEXTURE_FENCE_DESIGN.md)。

### `global_tools::loadPmemData`
- 平台工具函数：把 QNX pmem 设备缓冲映射为可交给纹理的句柄，避免 CPU 侧拷贝。
