# ImageDemo

- 对应源码：`samples/ImageDemo.cpp`
- 编译目标：`ImageDemo`
- 合并说明：旧 `ShadowDemo.md` 的内容（阴影能力）已并入本文

## Demo 用途

`MRImage` 图片组件的基础用法 + SSBO 批渲染验证：

- 一排 `MRImage` 共享同一纹理，验证合批（SSBO 实例化）是否生效；
- `Shadow` 组件为 `MRColor` / `MRImage` 添加投影；
- 内置自动化验收参数（`--report-json`），CI 可断言批渲染统计。

## 启动参数

| 参数 | 说明 |
|---|---|
| `--report-json` | 结束时输出批渲染统计 JSON，并执行合批验收断言（12 renderItems / 2 batches / 2 drawCalls），失败返回码 2 |
| `--request-render` | 启用按需渲染模式 |
| `--frames N` | 最多渲染 N 帧 |
| `--object-snapshot PATH` | 输出对象注册表快照 |
| `--object-snapshot-command PATH` | 对象快照命令文件 |

## 运行方式

```bash
cmake --build build --target ImageDemo --parallel 8
./build/ImageDemo.exe --report-json
```

## 示例做了什么

1. 以 `EngineOptions`（可关闭多线程、限制帧数）创建 `Engine`。
2. 加载 `assets/textures/img.bmp` 纹理，循环创建 10 个 `MRImage` 共享该纹理、
   不同高度摆放——同材质同纹理会被合入同一 SSBO batch。
3. `MRColor` 色块与 `MRImage` 各添加一个 `Shadow` 组件
   （`setShadowOffset(5,5)`、半透明黑），演示阴影投影。
4. `--report-json` 时输出 `batchStatistics` 并断言合批结果。

## 相关组件

### `MRImage`
- 图片组件：`setTexture(Texture)` 直接绑定纹理，或经
  `MeshRenderer::getMaterial()->setTexture("texture", ...)` 设置。

### `Shadow`
- 组件化投影：`addComponent<Shadow>()` 后设置 `setShadowOffset` / `setShadowColor`，
  可挂载到任意带 `MeshRenderer` 的组件（`shadow` shader 渲染底层投影）。

### 合批统计
- `FrameState::batchStatistics` 提供 renderItems/batches/ssboBatches/drawCalls 等
  计数，是验证 SSBO 批渲染（`isSSBOShader` + 材质兼容性 hash）是否生效的直接手段。
