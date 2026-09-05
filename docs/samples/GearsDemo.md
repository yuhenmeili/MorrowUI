# GearsDemo

- 对应源码：`samples/GearsDemo.cpp`
- 编译目标：`GearsDemo`
- 合并说明：由旧 `GearsIrisDemo.md`、`GearsOpeningDemo.md`、`GearsSelectDemo.md`、`GearsShineDemo.md` 合并而来

## Demo 用途

同屏对比展示四个仪表齿轮特效组件：`MRGearsIris`（光圈）、`MRGearsOpening`（展开）、
`MRGearsShine`（高光扫过）、`MRGearsSelect`（呼吸亮度选中点），每个组件配标签说明。

## 运行方式

```bash
cmake --build build --target GearsDemo --parallel 8
./build/GearsDemo.exe
```

## 示例做了什么

1. 深色背景 + 默认字体，顶部两行标题，四个区域各有组件名标签。
2. `MRGearsIris`：竖条光圈（144x760），`initialize()` 后播放。
3. `MRGearsOpening`：额外用 `Material::setTexture("texture", ...)` 换上
   `gearBG.png` 底图再 `initialize()`，演示特效组件的自定义纹理接入。
4. `MRGearsShine`：高光扫过效果（144x684）。
5. `MRGearsSelect`：10x10 的小尺寸选中点，呼吸亮度循环。

## 相关组件

### `MRGearsIris` / `MRGearsOpening` / `MRGearsShine` / `MRGearsSelect`
- 车机 HMI 常见的齿轮/光圈转场特效组件（对应 `gears_iris` / `gears_opening` /
  `gears_shine` / `gears_select` shader）。
- 统一模式：`create()` → 设置 `Transform` 位置尺寸 →（可选）替换材质纹理 →
  `initialize()` 启动内部动画。
