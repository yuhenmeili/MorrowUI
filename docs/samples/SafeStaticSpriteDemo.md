# SafeStaticSpriteDemo

- 对应源码：`samples/SafeStaticSpriteDemo.cpp`
- 编译目标：`SafeStaticSpriteDemo`

## Demo 用途

安全静态图片组件（`SafeStaticSprite`）演示，模拟仪表盘：三位数字时速表（0-999 循环）、
档位显示（P/R/N/D 轮播）、八个报警灯随机点亮/熄灭。图集以编译期 C 数组硬编码，
运行期零 IO、零动态分配。

## 运行方式

```bash
cmake --build build --target SafeStaticSpriteDemo --parallel 8
./build/SafeStaticSpriteDemo.exe
```

## 示例做了什么

1. 单线程 `Engine`，深色仪表盘背景。
2. **注册图集**：`StaticAtlasManager::getInstance().registerAtlas("hmi_main", kAtlasPixelData, 64, 32, kSpriteTable, count)`。
   图集像素数据由外部工具链（`tools/generate_atlas_c_array.py`）从 PNG 生成，
   `static_assert` 校验字节数；`kSpriteTable` 为硬编码的 `SafeSpriteDef` UV 表
   （speed_0~9 / gear_P,R,N,D / warning_* 共 22 个精灵）。
3. **数字时速表**：3 个 `SafeStaticSprite::create("hmi_main")`，`setSprite("speed_0")`
   初始化；`onFrameBegin` 中速度每 10 帧递增，仅在数字变化时调用 `setSprite`。
4. **档位**：单个精灵放大展示，P→R→N→D 每 30 帧切换。
5. **报警灯**：8 个图标每 15 帧以 60% 概率点亮（`setAlpha(1.0/0.15)`，
   内部有变更检测）。

## 相关组件

### `SafeStaticSprite` / `SafeStaticSpriteAtlas` / `StaticAtlasManager`
- 安全 HMI 图片组件族：图集数据编译期进入 `.rodata`、精灵名与 UV 静态绑定，
  `setSprite`/`setAlpha` 带变更检测，适合功能安全场景的仪表指示。
- 外部工具：`tools/generate_atlas_c_array.py` 把 PNG 图集转成 C 数组。
