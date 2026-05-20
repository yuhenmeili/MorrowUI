# TextDemo

- 对应源码：`samples/TextDemo.cpp`
- 编译目标：`TextDemo`

## Demo 用途

`TextDemo` 用来演示字体注册、文本内容设置和文本颜色设置，是最基础的文本渲染示例之一。当前内容同时包含英文和中文，因此也适合拿来检查多语言字形是否正常。

## 运行方式

```powershell
cmake --build "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw" --target TextDemo --config Debug -- -j 4
& "E:\WorkSpace\Client\morrow.gui\cmake-build-debug-mingw\TextDemo.exe"
```

## 示例做了什么

1. 创建窗口。
2. 注册字体 `../assets/fonts/MorrowSansCN1.1-Regular.otf`，名称为 `MorrowSansCN1.1-Regular.otf`。
3. 创建 `MRLabel`，设置位置与尺寸为 `512 x 512` 文本区域。
4. 设置文本内容 `Hello World!测试啦`。
5. 把文字颜色设为红色并加入窗口。

## 相关组件介绍

### `MRLabel`
- 文本渲染组件。
- 支持设置文字、颜色、对齐、自动换行、字符间距、行间距和最大行数等能力。
- 适合作为工程里所有静态文本和富布局文本的基础入口。

### `FontManager` / `Engine::addFonts`
- 字体需先注册到引擎，后续 `MRLabel` 才能按字体名取用。
- 这也是排查“文本不显示”问题时最先要检查的步骤。

### `Transform`
- 控制文本块的布局区域。
- 文本对齐、换行等都是在这个区域内发生的。

## 资源依赖

- 字体：`assets/fonts/MorrowSansCN1.1-Regular.otf`

## 适合继续扩展的方向

- 增加多种颜色、字号和对齐方式对比。
- 演示自动换行和最大行数裁切。
- 增加背景参考框，形成更完整的排版调试页面。

