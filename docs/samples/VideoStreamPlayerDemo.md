# VideoStreamPlayerDemo

- 对应源码：`samples/VideoStreamPlayerDemo.cpp`
- 编译目标：`VideoStreamPlayerDemo`

## Demo 用途

`MRVideoStreamPlayer` 视频流播放组件演示：使用内存 RGBA 帧源（程序化生成的动态
画面：游走的黄色圆 + 绿色扫描线），完整演示播放/暂停/停止、快进快退、倍速与
循环播放，状态和进度实时回显。

## 运行方式

```bash
cmake --build build --target VideoStreamPlayerDemo --parallel 8
./build/VideoStreamPlayerDemo.exe
```

## 示例做了什么

1. 深色背景 + 黑色圆角"播放器"底板。
2. `MRVideoStreamPlayer::create(640, 360, 30.0f, 300)` 创建 10 秒@30fps 的播放器，
   `setFrameProvider(generateFrame)` 注册帧生成回调（CPU 逐像素绘制，实际项目可
   替换为软件解码器或平台硬解码输出），`setLoop(true)` 循环。
3. **事件回显**：`events().onStateChanged`（PLAYING/PAUSED/STOPPED）更新状态标签；
   `events().onFrameChanged` 更新"时间 / 总时长 / 帧号"标签。
4. **控制按钮**：播放 `play()`、暂停 `pause()`、停止 `stop()`、
   `seekSeconds(±2)` 后退/前进、`setPlaybackSpeed` 切换 1x/2x。
5. 构造完成后立即 `play()` 开始播放。

## 相关组件

### `MRVideoStreamPlayer`
- 视频流播放组件：构造参数（宽、高、fps、总帧数）+ `setFrameProvider` 帧源 +
  `setLoop`；控制接口 `play/pause/stop/seekSeconds/setPlaybackSpeed`，
  查询 `getCurrentTime/getDuration/getPlaybackSpeed`；
  事件 `onStateChanged` / `onFrameChanged`。
- 帧源为内存 RGBA，亦可对接解码器输出；纯 OES 外部缓冲路径见
  [SafeStreamTextureDemo.md](SafeStreamTextureDemo.md)。
