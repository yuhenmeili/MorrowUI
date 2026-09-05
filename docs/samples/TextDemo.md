# TextDemo

- 对应源码：`samples/TextDemo.cpp`
- 编译目标：`TextDemo`
- 合并说明：由旧 `TextDemo.md`、`AlignmentDemo.md` 合并而来

## Demo 用途

文本能力集中展示：`MRTextEdit` 多行输入、`MRLineEdit` 单行/密码输入、
`MRLabel` 九宫格对齐、`MRRichTextLabel` 富文本标记。

## 运行方式

```bash
cmake --build build --target TextDemo --parallel 8
./build/TextDemo.exe
```

## 示例做了什么

1. **文本输入**（左半区）：
   - `MRTextEdit` 多行编辑（placeholder "按 Enter 换行"），
     `events().onTextChanged` 实时打印文本长度；
   - `MRLineEdit` 用户名输入（`setMaxLength(24)`），
     `events().onSubmitted` 回车提交；
   - `MRLineEdit` 密码输入（`setPasswordMode(true)`，maxLength 32）。
2. **Label 对齐**（右半区）：`addAlignmentSample` 用 `MRColor` 圆角底板 + `MRLabel`
   展示 6 种对齐组合——LEFT/CENTER/RIGHT × TOP/CENTER/BOTTOM，
   均为 `setAlign(horizontal, vertical)` 在同一固定区域内的效果。
3. **富文本**：`MRRichTextLabel` 演示标记语法——`[font_size=N]`、
   `[color=#RRGGBB]`、`[br]` 换行、标签嵌套，`setAutoWrap(true)` 按宽度自动换行、
   `setLineSpacing(1.15)` 行距。

## 相关组件

### `MRLabel`
- 基础文本标签：`setText(text, fontName)`、`setFontSize/setFontColor`、
  `setAlign(HorizontalAlignment, VerticalAlignment)`。

### `MRTextEdit` / `MRLineEdit`
- 多行 / 单行输入：placeholder、maxLength、字体字号；事件
  `onTextChanged`（实时）与 `onSubmitted`（回车）。
- 密码模式 `setPasswordMode(true)` 以圆点回显。

### `MRRichTextLabel`
- 富文本标签：`[font_size]` `[color]` `[br]` 标记可嵌套组合，
  `setAutoWrap` / `setLineSpacing` 控制排版。
