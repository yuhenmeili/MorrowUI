# 公共 API 泄漏基线（Phase 0 记录）

日期：2026-09-29（Phase 1 动工前）
扫描工具：`cmake/CheckPublicApiGLFree.cmake`（M1 门禁，报告模式）
扫描入口：`src/core/Engine.h`（quoted-include 传递闭包，仅解析 src/ 内头文件）

## 基线结果

闭包规模：**71 个头文件**。泄漏链与命中的内部文件：

| 泄漏文件 | 泄漏内容 |
|---|---|
| `renderer/resource/Shader.h` | include `OpenglHeader.h`/`GLESHeader.h`；`GLint`（ShaderPair/ShaderMap、fetch*、m_num*）；public `GLuint m_ID` |
| `renderer/resource/VertexArray.h` | include `OpenglHeader.h`/`GLESHeader.h`（签名已 Hw 化，纯遗留） |
| `renderer/device/RenderDeviceProxyBase.h` | include `GLRenderDevice.h`（泄漏链枢纽） |
| `renderer/device/GLRenderDevice.h` | include `EGLHeader.h`/`GLESHeader.h`（或 `OpenglHeader.h`）；`GLint m_viewportBeforeRenderTarget`；`GLuint` 参数 |
| `renderer/device/ShaderBinaryCache.h` | 注释级 GL 词汇（glProgramBinary、GL_VERSION 等） |
| `renderer/device/RenderDeviceOptions.h` | 注释级 GL 词汇（`glProgramBinary`） |
| `platform/wgl/OpenglHeader.h` | `glad/glad.h`、`GLFW/glfw3.h`（vendor 头全量类型/常量/函数声明） |
| `platform/egl/GLESHeader.h` | `<GLES3/gl31.h>` 等 GLES 系统头 |
| `platform/egl/EGLHeader.h` | `EGL/egl.h`、`EGL/eglextQCOM.h`、`EGL/eglext.h` |
| `includes/opengl/wgl/glad/glad.h`（经 OpenglHeader） | vendor GL 全量定义 |

传递路径（两条，均在公共头内）：

```text
Engine.h → Window.h → UIWidget.h → MeshRenderer.h → RenderDeviceProxyBase.h
         → GLRenderDevice.h → EGLHeader/GLESHeader（或 OpenglHeader → glad/GLFW）
Engine.h → ... → Material.h → RenderDeviceProxyBase.h / Shader.h → 同上
```

## Phase 1 验收对照

Phase 1 完成后重跑同命令（`API_GATE_FATAL=ON`），预期：闭包中无 GL/EGL/GLFW
词汇（vendor 头、GLRenderDevice、ShaderBinaryCache 不再出现在闭包内）。
`RenderDeviceOptions.h` 属于闭包内的合法公共配置头，其注释措辞同步去 GL 化。

## 已知非问题

- 闭包含 `morow` 自研枚举/句柄头（`DriverEnums.h`、`GpuTypes.h`、`ResourceHandle.h`
  等），设计上无 GL 依赖，扫描零命中；
- `RenderDeviceOptions.h`/`ShaderBinaryCache.h` 的命中均为注释文本，不构成编译期
  泄漏，但公共面不命名 GL API，Phase 1 一并改写（`RenderDeviceOptions.h`）；
  `ShaderBinaryCache.h` 属内部头，Phase 1 后退出闭包，不改。
