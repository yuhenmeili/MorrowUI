# GLTFDemo

- 对应源码：`samples/GLTFDemo.cpp`
- 编译目标：`GLTFDemo`

## Demo 用途

`GLTFDemo` 是当前工程里最完整的 3D 示例之一，用于在 UI 框架中加载并显示 GLTF/GLB 模型。它同时覆盖了这些能力：

- `MR3DSceneView` 离屏 3D 渲染并回贴到 UI
- `Scene3DAsyncLoader` 异步加载 3D 场景
- IBL 环境光照接入
- `OrbitController` 鼠标拖拽旋转和滚轮缩放
- 2D `MRButton` 与 3D 场景同屏共存

## 启动参数

```text
GLTFDemo [modelPath] [iblDirectory]
```

默认参数：

- `modelPath`：`../assets/models/2018_bmw_m5/scene.gltf`
- `iblDirectory`：`../assets/textures/ibl/symmetrical_garden_1k`

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target GLTFDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\GLTFDemo.exe"
```

指定模型和 IBL 目录：

```powershell
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\GLTFDemo.exe" "assets/models/2018_bmw_m5/scene.gltf" "assets/textures/ibl/symmetrical_garden_1k"
```

## 示例做了什么

1. 创建一个 `1920 x 1080` 窗口。
2. 创建 `MR3DSceneView`，配置场景清屏色、太阳光、环境光和 IBL。
3. 设置 `OrbitController` 的旋转/缩放速度与距离范围。
4. 设置 `OrbitCamera` 初始位置与 `lookAt`。
5. 创建 `Scene3DAsyncLoader`，异步加载 GLTF 模型。
6. 在 `onSceneBuilt` 中拿到 `sceneRoot`，把模型整体缩放到 `100x`。
7. 额外创建一个 `MRButton`，用于验证 2D 和 3D 交互共存。

## 相关组件介绍

### `MR3DSceneView`
- 一个把 3D 场景渲染到离屏 FBO，再合成回 UI 的组件。
- 提供 `setSceneRoot()`、`setSunLight()`、`setAmbientLight()`、`setIBLFromDirectory()`、`fitCameraToBounds()` 等能力。
- 是 2D/3D 混合渲染的核心入口。

### `Scene3DAsyncLoader`
- 一个通用的 3D 异步加载帮助类。
- 当前 demo 使用其 `loadGLTF()` 便捷接口，将后台解析和主线程 scene apply 分离开。
- 适合后续复用到其他 3D demo，而不只限于 GLTF。

### `OrbitController`
- 负责鼠标拖拽 orbit 和滚轮缩放。
- 可配置旋转速度、缩放速度、最小/最大距离。
- 当前事件架构下，它只会响应未被 2D widget 抢占的输入。

### `MRButton`
- 这里作为 2D 覆盖层存在。
- 主要用于验证按钮点击不会把拖拽穿透到 3D orbit 操作。

### `GLTFScene` / `SceneNode`
- `GLTFScene` 是解析后的场景数据。
- `SceneNode` 是真正挂到 `MR3DSceneView` 的运行时场景树。
- `onSceneBuilt` 是做后处理、缩放、挂动画组件的合适位置。

## 资源依赖

- 模型目录：`assets/models/2018_bmw_m5/`
- IBL 目录：`assets/textures/ibl/symmetrical_garden_1k`

## 交互说明

- 鼠标左键拖拽：旋转模型
- 鼠标滚轮：缩放
- 点击 2D 按钮：触发按钮回调，不应穿透到 3D orbit

## 适合继续扩展的方向

- 在 `onSceneBuilt` 中加入自动相机 fit 或 animation chooser。
- 增加模型切换下拉框，演示不同 GLTF 资源。
- 把 `Scene3DAsyncLoader` 进一步封装到更高层的 3D widget API 中。

