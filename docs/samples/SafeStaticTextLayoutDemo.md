# SafeStaticTextLayoutDemo

- 对应源码：`samples/SafeStaticTextLayoutDemo.cpp`
- 编译目标：`SafeStaticTextLayoutDemo`

## Demo 用途

安全静态文本排版器（`SafeStaticTextLayout`）演示，模拟仪表盘强提示告警：
三行告警词句（红/橙/黄），周期性轮播第三行文本，字符间距与颜色可调。
自建 37 字符（A-Z / 0-9 / 空格）的 8x12 像素"字体"图集，全流程无动态字体加载。

## 运行方式

```bash
cmake --build build --target SafeStaticTextLayoutDemo --parallel 8
./build/SafeStaticTextLayoutDemo.exe
```

## 示例做了什么

1. **构建字体图集**：`kFontAtlasData`（512x16 RGBA）在 `initFontAtlas()` 中用代码填充
   （白色边框 + 位置标记线区分字符）；`buildCharSpriteTable()` 生成 37 个
   `SafeSpriteDef`（名称指向 `.rodata` 字符串字面量）。
2. **注册图集**：`StaticAtlasManager::getInstance().registerAtlas("font_hmi", ...)`。
3. **三行告警**：`SafeStaticTextLayout::create("font_hmi")` × 3，
   `setColor` 设红/橙/黄，`setText(data, length)` 显示
   "CHECK ENGINE" / "BRAKE FAILURE" 等固定词句。
4. **动态轮播**：`onFrameBegin` 中第三行周期性切换文本；文本 → 精灵名通过
   `charToSpriteName` 查表转换，无动态分配。

## 相关组件

### `SafeStaticTextLayout`
- 安全 HMI 文本组件：基于预注册的静态图集逐字符排版，接口为
  `create(atlasName)` / `setText(const char*, int)` / `setColor`。
- 与 `SafeStaticSprite` 共用 `StaticAtlasManager` 图集体系，适合告警词、
  档位符号等有限字符集的功能安全文本。
