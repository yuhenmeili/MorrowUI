# PMemoryDemo

- 对应源码：`samples/PMemoryDemo.cpp`
- 编译目标：`PMemoryDemo`

## Demo 用途

`PMemoryDemo` 演示如何在 QNX 条件下把 PMEM 数据接入为 OES 纹理并显示到 `MRImage`。这个 demo 更偏平台能力验证，而不是通用 UI 展示。

## 重要说明

当前 `main()` 里的实际逻辑被 `#ifdef QNX` 包住，因此：

- 在 QNX 平台上会执行 PMEM 纹理加载与显示
- 在非 QNX 平台上程序基本直接返回

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target PMemoryDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\PMemoryDemo.exe"
```

> 说明：上面的命令只给出目标构建方式。真正看到 PMEM/OES 效果，需要对应 QNX 运行环境和底层平台支持。

## 示例做了什么

1. 在 QNX 条件下创建 `Engine("qnx")`。
2. 调用 `global_tools::loadPmemData()` 读取 `../assets/textures/d_p.rgb` 到 PMEM。
3. 创建 `Texture::create(ImageType::OES)`。
4. 如果数据加载成功，调用 `setTextureData()` 把 PMEM 数据包装为纹理。
5. 创建 `MRImage` 并添加到窗口。

## 相关组件介绍

### `global_tools::loadPmemData`
- 用于把外部 RGB 数据读入 PMEM 句柄。
- 是这个 demo 的平台能力入口。

### `Texture(ImageType::OES)`
- 用于接入外部纹理或平台原生图像缓冲。
- 与普通 `IMAGE` 纹理不同，它更贴近底层平台纹理源。

### `MRImage`
- 负责把得到的纹理数据显示在 UI 上。
- 说明 `MRImage` 不仅能显示普通文件贴图，也能承接平台纹理源。

### `Transform`
- 用于设置显示尺寸。
- 当前示例只设置了大小，没有额外设置位置，因此会按默认坐标显示。

## 资源依赖

- 原始数据：`assets/textures/d_p.rgb`

## 适合继续扩展的方向

- 补充一个非 QNX 的 fallback 路径，便于桌面环境调试。
- 在文档里继续补齐 PMEM 数据格式和 stride 约束说明。
- 增加动态刷帧案例，验证视频帧或摄像头帧接入。

