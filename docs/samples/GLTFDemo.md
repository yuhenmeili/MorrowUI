# GLTFDemo

- 对应源码：`samples/GLTFDemo.cpp`
- 编译目标：`GLTFDemo`

## Demo 用途

在 UI 框架中加载并显示 GLTF/GLB 模型，是 3D 能力的完整示例：

- `MR3DSceneView` 离屏 3D 渲染并回贴到 UI
- `Scene3DAsyncLoader` 异步加载 3D 场景（不阻塞渲染线程）
- IBL 环境光照 + 平行光/环境光设置
- `OrbitController` 鼠标拖拽旋转、滚轮缩放
- 2D `MRButton` 与 3D 场景同屏共存

## 启动参数

```text
GLTFDemo [modelPath] [iblDirectory]
```

默认参数：

- `modelPath`：`assets/models/2018_bmw_m5/scene.gltf`
- `iblDirectory`：`assets/textures/ibl/symmetrical_garden_1k`

## 运行方式

```bash
cmake --build build --target GLTFDemo --parallel 8
./build/GLTFDemo.exe
```

## 示例做了什么

1. 以 `EngineOptions`（1920x1080、`samples = 4` 多重采样）创建 `Engine`。
2. 创建 `MR3DSceneView`，设置场景底色、太阳光（方向/颜色/强度）、环境光，
   并用 `setIBLFromDirectory` 接入 IBL 预计算输出目录。
3. 配置 `OrbitController` 速度与距离范围，相机初始位于 `(0, 3, 10)` 看向原点。
4. `Scene3DAsyncLoader::create(engine, scene3DView)` 异步加载模型：
   `loadOptions.sceneOptions.cameraFit` 自动取景（padding 1.25），加载完成在
   `onSceneBuilt` 回调打印节点/网格/动画数量。
5. 添加一个 2D `MRButton`，验证 3D 场景之上仍可正常叠加 UI 控件。

## 相关组件

### `MR3DSceneView`
- 内嵌离屏 3D 视口的 UI 组件：光照设置、IBL 接入、`getOrbitCamera()` /
  `getOrbitController()` 相机控制。

### `Scene3DAsyncLoader`
- GLTF 异步加载器：`loadGLTF(path, options)`，`onSceneBuilt` /
  `sceneOptions.onError` 回调，`cameraFit` 自动包围取景。

### IBL 数据来源
- 由 `IBLPrecomputeDemo`（`IBLPrecompute::bake`）离线生成，见
  [IBLPrecomputeDemo.md](IBLPrecomputeDemo.md)。
