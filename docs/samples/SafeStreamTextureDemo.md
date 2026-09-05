# SafeStreamTextureDemo

- 对应源码：`samples/SafeStreamTextureDemo.cpp`
- 编译目标：`SafeStreamTextureDemo`

## Demo 用途

安全流媒体纹理组件（`SafeStreamTexture`）演示，模拟 RVC 倒车影像：CPU 生成
SMPTE 七彩条测试图 + 绿色扫描线 + 黄色倒车轨迹虚线（角度随正弦摆动），
以 30fps 逐帧更新纹理，演示多缓冲轮转的流式上传。

## 运行方式

```bash
cmake --build build --target SafeStreamTextureDemo --parallel 8
./build/SafeStreamTextureDemo.exe
```

## 示例做了什么

1. 单线程 `Engine`，深色背景。
2. `SafeStreamTexture::create(640, 480)` 创建流纹理组件，按窗口等比缩放摆放。
3. `generateTestPattern(frame, buffer)` 在 CPU 端生成测试画面（彩条 + 辅助线 +
   轨迹线，全部像素级合成）。
4. `onFrameBegin` 中使用 **3 个轮转缓冲**写入帧数据后提交；所有缓冲忙碌时打
   `LOG_W` 跳过该帧（背压保护，不阻塞渲染线程）。

## 相关组件

### `SafeStreamTexture`
- 面向安全场景的视频流纹理组件：固定尺寸创建、CPU 帧缓冲写入 + 提交的
  流式更新模式，内部多缓冲（OES）轮转避免读写冲突。
- 典型来源：倒车影像（RVC）、 surveillance 流；硬解码输出可对接平台缓冲
  （参考 [PMemoryDemo.md](PMemoryDemo.md) 的 pmem 路径）。
