# Shader 源码组织与统一 Instance 布局方案

> **决策记录（2026-09-04）**：确认采用统一 `instance.glsl` SSBO 布局——全部 UI 组件
> 共用一个通用 `InstanceData` 结构；CPU 侧每帧**全量填充**，未使用的属性槽位写入
> **默认值**。接受由此产生的显存/带宽冗余，换取架构简化。

> **实施状态（2026-09-05）**
>
> - P0（死文本清理）、P1（include 机制 + CPU 统一布局）、P2（22 个 shader 全量迁移）、
>   P3（`#pragma morrow ssbo` 探测 + `m_defines` 拆分）**已全部实施完成**，见第 9 节 ✅ 标记；
> - 与文档的两处实现差异（见 5.1 / 6.1 节备注）：
>   1. "默认值先行写入"通过**每个 layout 声明全部 6 个槽位**（未使用槽位用常量分量）实现，
>      而非 defaultData 模板 memcpy——保证相同（任何槽位任何帧都有确定值），机制更简单，
>      且 reflection 校验保持 1:1 字段对应；
>   2. common/ 为 6 个文件：新增 `instance_accessors.glsl` 作为 vert/frag 两个入口 chunk
>      共享的单一 accessor 来源，避免 accessor 文本双份维护；
> - 验证：P0 清理后两个平台的最终编译产物与清理前**逐字节一致**（模拟注入管线比对）；
>   `SSBOLayoutTests` 扩展至 30 个用例（统一布局填充 / include 解析 / uniform 打包 /
>   reflection 校验），全仓 26 项 ctest 全部通过。

## 1. 背景

引擎的通用 shader（`assets/shaders/*.vert / *.frag`）目前采用"单文件双变体"的组织方式：
每个支持 SSBO 批渲染的 shader 在同一个文件里用 `#ifdef ENABLE_SSBO` 手写两份实现——
批渲染路径（从 SSBO 取实例数据）和单绘制路径（读 uniform）。`Material::buildShader(bool enableSSBO)`
在编译前向源码头注入 `#define ENABLE_SSBO` 来选择变体。

### 1.1 现状量化（2026-09-04 测量）

`assets/shaders/` 共 52 个 `.vert/.frag` 文件、2019 行，其中：

| 重复项 | 数量 | 说明 |
|---|---|---|
| `#ifdef ENABLE_SSBO` 双分支文件 | 22（11 family × 2 阶段） | 每个文件内 main() 逻辑写两遍 |
| `struct InstanceData` 声明 | 22 处 | 每个 family 的 .vert 和 .frag 各声明一遍 |
| vert 的 batchID/模型变换样板 | 11 处 | `int batchID = int(floor(a_batch + 0.1))` 到 `#else` 回退，逐字重复 |
| `layout(std140) uniform Global` 块 | 11 处 | 内容完全相同 |
| 圆角 discard 块 | 5 处 frag | 约 10 行逐字相同 |
| 内联 `#version 320 es` | 26 处 | 运行时 `normalizeShaderVersion()` **必然替换**，属于死文本 |
| 内联 `precision mediump ...` | 20 处 | 使运行时自动注入失效，属于双份维护 |

11 个支持 SSBO 的 family 及其实例数据（现状各不相同）：

| family | InstanceData 字段 | 大小 |
|---|---|---|
| image_normal / image_oes / image_text_debug | model, displaySize(xy), imageAttr(rounding, alpha) | 96B |
| default_color | model, defaultColor, defaultAttr(displaySize.xy, rounding, alpha) | 96B |
| default_image / texture_button / anchor_point_scale | model, defaultAttr(alpha) | 80B |
| font | model, fontColor, fontAttr(alpha) | 96B |
| button | model, bgColor, defaultAttr(alpha), textureAttr(useTexture) | 112B |
| bounce | model, meshCenter.xyz+alpha, defaultAttr(4 个动画参数) | 96B |
| progress_bar | model, trackColor, fillColor, defaultAttr(displaySize/rounding/alpha), stateAttr(progress 等 4 个) | 128B |

其余 15 个（particle/gltf/shadow/特效类）为单变体 shader，不在本方案范围内。

### 1.2 与 SSBO_LAYOUT_AUTOMATION.md 的关系

`docs/SSBO_LAYOUT_AUTOMATION.md`（P0~P3 已落地）解决 **CPU 侧**问题：SSBO 布局注册、
声明式字段绑定、shader reflection 校验。本方案解决 **GLSL 源码侧**问题，并在其上
更进一步：**统一实例布局**后，CPU 侧的 11 个 per-shader layout 注册收敛为 1 个，
声明式绑定成为唯一的"属性名 → 槽位"映射来源，两端同时简化。

---

## 2. 确认的决策

| 决策点 | 结论 |
|---|---|
| 实例布局 | 全部 UI 组件共用一个通用 `InstanceData`（`common/instance.glsl` 唯一定义） |
| 填充策略 | 每帧**全量填充**所有槽位；组件未提供的属性写默认值（不做按需填充） |
| accessor | 全局统一一套（`instanceAlpha()`、`instanceColor()` 等），各 shader 不再自定义 |
| 非 SSBO 变体 | 同样统一为通用 uniform 集（`u_color0` / `u_geomAttr` / ...），由同一份绑定表打包 |
| 变体机制 | 保持编译期 `#ifdef` + 两次 `createGPUProgram`，不做 uber shader |
| 工具链 | 不引入 glslang/SPIR-V/Python 依赖；约 40 行 C++ 运行时 include 解析 |

**为什么"全量填充 + 默认值"而不是"按需填充"**：instance buffer 是跨帧复用的，
若 filler 只写组件用到的字段，其余槽位会**残留上一帧或其他实例的旧数据**——组件
动态增删属性时出现难查的闪烁/鬼影。全量填充把"清零责任"放进 filler 本身，任何
槽位任何帧都有确定值，行为可预测、可断言。

---

## 3. 目标与约束

### 3.1 目标

1. `InstanceData` 全引擎唯一定义（1 处替代 22 处），.vert/.frag 一致性由编译器保证；
2. 全局统一 accessor 集，shader 本体只剩 include + 一份 main()，零 `#ifdef`；
3. CPU 侧 11 个 SSBO layout 注册收敛为 1 个通用 layout + 每 shader 一份字段绑定声明；
4. 新增支持批渲染的 shader：include 两个 chunk、按槽位填绑定，即完成；
5. 公共逻辑（圆角裁剪等）修改只改一处。

### 3.2 约束（引擎定位决定）

1. **极致轻量、车载端（QNX/Linux）**：不引入新构建工具；解析器必须在 C++ 侧存在
   （开发期文件回退路径要用），嵌入路径直接复用同一个运行时解析器；
2. 产物仍是纯 GLSL ES 3.2 / 桌面 GL 4.6 源码，驱动直编；
3. 零每帧额外开销（include 解析仅 `loadShader()` 一次）；
4. 保留两条加载路径：`EmbeddedShaders` 嵌入 + `assets/shaders/` 文件回退。

---

## 4. 统一 InstanceData 设计

### 4.1 结构定义（`common/instance.glsl`，全引擎唯一）

```glsl
#ifdef ENABLE_SSBO
struct InstanceData {
    mat4 model;       //世界变换矩阵 <- Transform::getWorldMatrix()
    vec4 color0;      //主色：bgColor / fontColor / defaultColor / trackColor ...
    vec4 color1;      //副色或辅助向量：fillColor / meshCenter.xyz + alpha
    vec4 geomAttr;    //x,y = displaySize；z = rounding；w = alpha
    vec4 stateAttr;   //状态开关/进度：useTexture / progress / direction / ...
    vec4 extraAttr;   //自定义参数区：动画参数等
};
layout (std430, binding = 0) buffer InstanceBuffer {
    InstanceData instances[];
};
#endif
```

- 大小：`mat4(64B) + 5 × vec4(80B) = 144B`，std430 下无 padding；
- 5 个 vec4 槽位是 11 个 family 现有字段的**超集**（最大现状 progress_bar 已是 128B，
  统一后仅 +16B；最小的 80B family 增至 144B，+64B）。

### 4.2 family → 槽位映射表

"—"表示该 family 不使用此槽位，全量填充时写默认值：

| family | color0 | color1 | geomAttr | stateAttr | extraAttr |
|---|---|---|---|---|---|
| image_normal / image_oes / image_text_debug | — | — | displaySize.xy / rounding / alpha | — | — |
| default_color | defaultColor | — | displaySize.xy / rounding / alpha | — | — |
| default_image / texture_button / anchor_point_scale | — | — | w = alpha | — | — |
| font | fontColor | — | w = alpha | — | — |
| button | bgColor | — | w = alpha | x = useTexture | — |
| bounce | — | meshCenter.xyz + w=alpha | — | — | timeDelta / duration / bounceTimes / scaleRange |
| progress_bar | trackColor | fillColor | displaySize.xy / rounding / alpha | progress / direction / useTrackTexture / useFillTexture | — |

槽位语义刻意保持"弱类型"（color0/geomAttr/stateAttr 而非 bgColor/rounding）：
槽位是**存储协议**，语义由每 shader 的绑定声明给出。这与 SSBO_LAYOUT_AUTOMATION.md
§3.4 对 `attrN` 的批评并不矛盾——该批评针对"无任何语义的纯序号命名"；本方案的
槽位有明确分类语义，且绑定表提供了精确到分量的业务映射。

### 4.3 冗余开销量化

| 项 | 估算 | 结论 |
|---|---|---|
| 单实例增大 | 80B family：+64B；128B family：+16B | 可接受 |
| 满屏 500 实例的填充量 | 72KB/帧（现状约 40~64KB） | CPU memcpy 量级，微秒级 |
| 60fps 带宽 | ≈ 4.3MB/s 写 | 车载 SoC 带宽的万分之几 |
| 显存驻留 | SSBO 按 batch 分配、跨帧复用 | 峰值 +80%（最小 family），绝对值 KB 级 |

结论：以 KB 级显存和微量带宽，换取下述架构简化，符合引擎"轻量优先、规模可控
（UI 组件同构、数量有限）"的定位。

---

## 5. 全量填充与默认值（CPU 侧核心语义）

### 5.1 默认值表

| 槽位 | 默认值 | 理由 |
|---|---|---|
| model | 无默认，必填 | 来自 Transform 世界矩阵，恒有效 |
| color0 / color1 | `vec4(1, 1, 1, 1)` | 乘法中性色，alpha=1 不透明 |
| geomAttr | `vec4(0, 0, 0, 1)` | 零尺寸、无圆角、不透明 |
| stateAttr | `vec4(0, 0, 0, 0)` | 所有开关关闭 |
| extraAttr | `vec4(0, 0, 0, 0)` | 参数为零 |

默认值在**通用 filler 入口处先整块写入**（memcpy 一个全默认的 `InstanceData` 模板，
`model` 除外），再覆盖绑定表声明的字段。这保证"未声明字段 = 确定默认值"，与
"绑定表漏写"错误解耦。

> **实现备注（2026-09-05）**：实际实现未引入 defaultData 模板，而是让**每个 layout
> 声明全部 6 个槽位**——未使用的槽位用 `constantFloat` 常量分量表达默认值
> （`defaultColorSlot()` / `defaultGeomSlot()` / `zeroSlot()` 等构造辅助）。效果等价：
> 任何槽位任何帧都被写入确定值，回收池脏数据（`ShaderStorageBuffer::resize` 复用
> 旧 buffer 不清零）不可能泄漏到渲染；且字段集与 GLSL 结构 1:1，reflection 校验
> 保持严格匹配。SSBOLayout / fillSSBOInstance 机制零改动。

### 5.2 通用 layout 注册（收敛为一条）

SSBOManager 侧 11 个 per-shader factory 收敛为 1 个：

```cpp
struct UIInstanceData {                 // 与 GLSL 一一对应
    Matrix4 model;                      // offset 0
    Vector4 color0;                     // offset 64
    Vector4 color1;                     // offset 80
    Vector4 geomAttr;                   // offset 96
    Vector4 stateAttr;                  // offset 112
    Vector4 extraAttr;                  // offset 128
};                                      // sizeof = 144
static_assert(sizeof(UIInstanceData) == 144);
static_assert(std::is_standard_layout_v<UIInstanceData>);
// ... offsetof 断言略，同 SSBO_LAYOUT_AUTOMATION.md §10.2
```

每 shader 只剩一份**声明式字段绑定**（复用已落地的 P2 机制），描述"哪个 Material
属性填哪个槽位的哪个分量"。以 `image_normal` 为例：

```cpp
bindings["image_normal"] = {
    worldMatrix("model"),
    packedVec4("geomAttr", {vecC("displaySize", 0), vecC("displaySize", 1),
                            matFloat("rounding"), matFloat("alpha")}),
};
// color0/color1/stateAttr/extraAttr 未声明 → 写默认值
```

`progress_bar` 声明 4 个槽位，`bounce` 用 `color1` 装 meshCenter+alpha、
`extraAttr` 装动画参数——绑定表完整表达了 4.2 节的映射。

### 5.3 reflection 校验简化

布局统一后，`validateSSBOLayout`（SSBO doc P3）只需针对**一个结构**写一次期望值
（字段名、offset、类型、stride），所有 shader 的批渲染变体共用该校验；新增 shader
零额外校验代码。

---

## 6. 统一 accessor 与公共块库

### 6.1 目录结构

```text
assets/shaders/
├── common/                      # 公共块库（共 6 个文件）
│   ├── global.glsl              #   Global UBO（projectionView）
│   ├── instance.glsl            #   统一 InstanceData —— 唯一定义
│   ├── instance_accessors.glsl  #   共享 accessor（vert/frag 两入口单一来源）
│   ├── instance.vert.glsl       #   顶点阶段入口（a_batch → 实例索引/模型矩阵）
│   ├── instance.frag.glsl       #   片段阶段入口（v_batchID → 实例索引）
│   └── rounded_clip.glsl        #   圆角裁剪函数
├── image_normal.vert            # 只剩 include + 一份 main()
├── image_normal.frag
└── ...
```

> `#ifdef ENABLE_SSBO` 全引擎仅存在于 `instance.glsl`、`instance.vert.glsl`、
> `instance_accessors.glsl` 三个 chunk 中。`instance_accessors.glsl` 依赖宿主
> 阶段先定义 `instanceBatchID()`（vert 从 `a_batch` 计算，frag 读 `v_batchID`），
> 因此同一份 accessor 文本可在两个阶段复用。

### 6.2 顶点阶段 accessor（`common/instance.vert.glsl`）

`#ifdef ENABLE_SSBO` 只存在于这里和 6.3，全引擎仅此两处：

```glsl
#include "common/instance.glsl"

#ifdef ENABLE_SSBO
int instanceBatchID() {
    return int(floor(a_batch + 0.1));
}
mat4 instanceModel() {
    return instances[instanceBatchID()].model;
}
#else
uniform mat4 u_model;
int instanceBatchID() { return 0; }
mat4 instanceModel()  { return u_model; }
#endif
```

### 6.3 片段阶段 accessor（`common/instance.frag.glsl`）

```glsl
#include "common/instance.glsl"

#ifdef ENABLE_SSBO
vec4  instanceColor()       { return instances[v_batchID].color0; }
vec4  instanceColor1()      { return instances[v_batchID].color1; }
vec2  instanceDisplaySize() { return instances[v_batchID].geomAttr.xy; }
float instanceRounding()    { return instances[v_batchID].geomAttr.z; }
float instanceAlpha()       { return instances[v_batchID].geomAttr.w; }
vec4  instanceState()       { return instances[v_batchID].stateAttr; }
float instanceUseTexture()  { return instances[v_batchID].stateAttr.x; }
vec4  instanceExtra()       { return instances[v_batchID].extraAttr; }
#else
uniform vec4 u_color0;
uniform vec4 u_color1;
uniform vec4 u_geomAttr;    // displaySize.xy, rounding, alpha
uniform vec4 u_stateAttr;
uniform vec4 u_extraAttr;
vec4  instanceColor()       { return u_color0; }
vec4  instanceColor1()      { return u_color1; }
vec2  instanceDisplaySize() { return u_geomAttr.xy; }
float instanceRounding()    { return u_geomAttr.z; }
float instanceAlpha()       { return u_geomAttr.w; }
vec4  instanceState()       { return u_stateAttr; }
float instanceUseTexture()  { return u_stateAttr.x; }
vec4  instanceExtra()       { return u_extraAttr; }
#endif
```

`common/rounded_clip.glsl`（5 处重复的收敛）：

```glsl
void applyRoundedClip(vec2 displaySize, float rounding, vec3 localPosition) {
    if (rounding <= 0.0)
        return;
    vec3 center = vec3(displaySize.x / 2.0 - rounding, displaySize.y / 2.0 - rounding, localPosition.z);
    vec3 current = vec3(abs(localPosition));
    float distanceSq = dot(current - center, current - center);
    float roundingSq = rounding * rounding;
    if (current.x > center.x && current.y > center.y && distanceSq > roundingSq)
        discard;
}
```

### 6.4 改造后的 shader 本体

`image_normal.frag` 全文（原 47 行双分支 → 单分支）：

```glsl
layout (location = 0) in vec3 v_position;
layout (location = 1) flat in int v_batchID;
layout (location = 2) in vec4 v_color;
layout (location = 3) in vec2 v_texCoord;

#include "common/instance.frag.glsl"
#include "common/rounded_clip.glsl"

uniform sampler2D u_texture;

layout (location = 0) out vec4 fragColor;
void main() {
    applyRoundedClip(instanceDisplaySize(), instanceRounding(), v_position);
    fragColor = texture(u_texture, v_texCoord.st);
    fragColor.a *= instanceAlpha();
}
```

`progress_bar.frag` 关键行（两个颜色 + 状态槽位直接可用）：

```glsl
vec4 trackColor = instanceColor();
vec4 fillColor  = instanceColor1();
float progress  = instanceState().x;
```

`bounce.vert` 关键行（辅助向量 + 自定义参数区）：

```glsl
vec3 meshCenter = instanceColor1().xyz;
float timeDelta = instanceExtra().x;
```

所有 accessor 为 1~3 行平凡函数，GLSL 编译器必然内联，SSBO 路径生成的机器码与
手写展开一致；变体选择仍是编译期宏，运行时零分支。

### 6.5 非 SSBO 变体的 uniform 打包

现状各 family 的 uniform 名互不相同（`u_color` / `u_fontColor` / `u_trackColor` /
`u_useTexture` ...），统一后 GLSL 只声明 6.3 节的通用 uniform 集。**组件侧 Material
属性名不变**（仍是 `setFloat("alpha")`、`setVector("color")` 等），由 5.2 节的
**同一份绑定表**在非 SSBO 路径做打包：按绑定把属性值拼成 `u_geomAttr` 等 vec4 后
`glUniform4f`。即：

```text
同一份声明式绑定
  ├─ SSBO 路径  → filler 全量填充 instance buffer
  └─ 非 SSBO 路径 → 打包为 u_color0 / u_geomAttr / ... 后设置 uniform
```

绑定表因此成为"属性名 → 槽位"的唯一事实来源，两条渲染路径不会分叉。
（GLSL 中未被某 shader 引用的 uniform 会被编译器剔除，`glUniform` 对无效
location 的调用按 GL 规范静默忽略，无需逐 shader 特判。）

---

## 7. include 解析机制

### 7.1 语义（刻意保持最小）

```glsl
#include "common/instance.glsl"
```

- 仅支持行首 `#include "..."`，路径相对 `assets/shaders/` 根；
- **include-once**：同一文件全局只展开一次（解析器去重，防重定义）；
- 循环包含 / 文件缺失 → `LOG_E` 并置空源码（fail-fast）；
- 嵌套深度限制 8 层；
- 整段文本拼接，拼接后行号连续，驱动报错行号仍然准确。

### 7.2 实现（约 40 行，MaterialUtil 新增）

```cpp
// resolveIncludes(source, loader, depth)
// 逐行扫描；命中 #include：去重集合判断 → loader(path)
// （先 embedded_shaders::get，再 assets/shaders/ 文件）→ 递归解析拼接。
```

接入点：`Material::loadShader()` 读到源码后立即解析，将**解析后**的源码存入
`m_vertexShaderResource / m_fragmentShaderResource`。`buildShader()`、
`isSSBOShader()` 等下游零改动；`setShaderFromMemory()` 无 include 行时行为不变。

构建侧唯一改动：`cmake/EmbedShaders.cmake` 的 GLOB 增加
`"${SHADER_DIR}/common/*.glsl"`。

---

## 8. 明确不统一的部分

- **非 SSBO 的 15 个特效 shader**（particle/gltf/shadow/scene3d/gears 等）：单变体、
  无批渲染需求，只做 P0 死文本清理，不迁统一布局；
- **varying/attribute 声明块**：各 shader 属性组合差异大（font 无 a_color、
  particle 自有属性），不强行收敛；仅 image 三兄弟相同，可后续按需评估；
- **family 内的业务差异保留在各自 main()**：如 `image_oes.frag` 乘 `v_color` 而
  `image_normal.frag` 不乘——chunk 只提供数据，main 永远属于 shader 自己；
- `image_oes.frag` 的 `GL_OES_EGL_image_external` extension 行保留不动。

---

## 9. 实施步骤

### P0：死文本清理（零风险，可立即做）✅（2026-09-05）

1. 删除 26 处内联 `#version 320 es`（运行时必然替换）；—— 已删（gears/gltf/scene3d
   保留各自的非标准 precision，image_oes.frag 保留 extension 顺序所需的内联 precision）
2. 删除与运行时注入等价的 precision；—— 已删（精确等于注入对的 20 处）

验证：对比改造前后 `buildShader()` 送入驱动的字符串，逐字节一致。—— 已用脚本模拟
注入管线（normalize + precision 注入 + head 拼接）对全部 52 个文件 × 2 个平台版本
做 md5 比对，全部一致。

### P1：机制先行（解析器 + CPU 侧统一布局）✅（2026-09-05）

1. `MaterialUtil::resolveIncludes()` + `EmbedShaders.cmake` GLOB 扩展；—— 已实现
   （循环包含用"完成集 + 进行中栈"双集合检测，不被 include-once 吞掉）
2. 新建 `common/` chunk；—— 6 个文件（见 6.1 节实现备注）
3. CPU 侧：`UIInstanceData` + 全槽位声明绑定；—— 已实现（见 5.1 节实现备注）
4. 非 SSBO 路径的绑定驱动 uniform 打包（6.5 节）；—— `packSSBOLayoutUniforms()`
   接入 `BatchManager::renderStandardBatch`
5. 单元测试；—— `SSBOLayoutTests` 30 用例全过

### P2：GLSL 全量迁移（收益主体）✅（2026-09-05）

11 个 family × 2 阶段共 22 个文件一次性迁移完成（include 展开 + 单分支 main +
`#pragma morrow ssbo` 标记）。bounce.frag 顺带将 `texture2D` 现代化为 `texture()`
（语义相同，ES 3.2 / core 4.6 双端合法）。

### P3：加固 ✅（2026-09-05）

1. `isSSBOShader()` 显式标记探测；—— `#pragma morrow ssbo` 优先，ENABLE_SSBO
   文本回退（兼容 setShaderFromMemory 自定义 shader）
2. `m_defines` 拆分；—— ENABLE_SSBO 由 `buildShader(enableSSBO)` 参数注入，
   业务宏两个变体一致注入
3. 开发期启动自检；—— 未实施（可选，后续按需补充）

---

## 10. 预期收益

| 指标 | 现状 | 迁移后 |
|---|---|---|
| InstanceData 定义 | 22 处手工拷贝，靠人工同步 | **1 处**（instance.glsl），一致性由编译器保证 |
| GLSL `#ifdef ENABLE_SSBO` | 22 个 shader 文件 | **2 个 chunk 文件** |
| accessor | 各 family 各写一套（imageDisplaySize 等） | 全局统一一套（8+2 个函数） |
| 圆角 discard | 5 处逐字重复 | 1 处 |
| vert 变换样板 | 11 处 | 0 处（收进 instance.vert.glsl） |
| CPU layout 注册 | 11 个 per-shader factory | **1 个通用 layout + 每 shader 一份绑定声明** |
| reflection 校验 | 按 layout 分散 | 单结构一份期望值，全局共用 |
| 新增 SSBO shader | 写两遍分支 + 手抄结构体 + 注册 layout | include chunk + 声明绑定（GLSL 约 10 行 + 绑定约 5 行） |
| 公共逻辑修改（圆角抗锯齿化等） | 改 5 个文件 | 改 1 个文件 |
| 显存/带宽代价 | — | 单实例最大 +64B；满屏 +KB 级流量（4.3 节） |

源码行数预计下降 35%~45%（2019 行 → 约 1150 行）；更重要的是"新增/修改"路径的
出错面从"GLSL 两遍 × 22 + CPU 一份对齐"缩小为"chunk 一处 + 绑定一份声明"。

## 11. 风险与缓解

| 风险 | 缓解 |
|---|---|
| 全量填充的冗余开销 | 已量化（4.3 节）：KB 级显存、微量带宽；若未来出现极端实例数场景，可按 family 注册"槽位裁剪布局"作为逃生通道（机制向后兼容） |
| 按槽位语义弱化导致误用（把动画参数写进 color0 等） | 绑定表集中声明 + 默认值表 + 单测覆盖每个 family 的映射；reflection 校验兜底 |
| include 解析器 bug | 极小、纯 CPU、可单测；fail-fast；P1 先以 global/rounded_clip 验证机制 |
| 非 SSBO 路径 uniform 打包回归 | 打包逻辑由绑定表驱动并与 SSBO filler 共用同一声明，P1 单测覆盖；迁移期逐 family 双变体 diff |
| 迁移期新旧布局并存的一致性 | P1 起 CPU 新旧并存但行为不变；P2 每 family 迁移即删旧注册，不长期共存 |
| 误收敛刻意差异（image_oes 乘 v_color 等） | 第 8 节明确"main 属于 shader 自己"，迁移时逐 family 对照旧源码 |
| QNX 构建环境 | 零新工具依赖；构建侧仅改一行 GLOB |

## 12. 结论

本方案在渲染架构完全不变的前提下完成三层收敛：

```text
统一 InstanceData（1 处定义，全量填充 + 默认值）
  + 统一 accessor（#ifdef 封闭在 2 个 chunk，shader 一份 main）
  + 统一绑定表（SSBO 填充与非 SSBO uniform 打包共用同一声明）
```

与已落地的 SSBO_LAYOUT_AUTOMATION.md（CPU 侧注册表 + 字段绑定 + reflection 校验）
衔接后，"支持批渲染的材质"的完整成本从 *GLSL 手写两遍 × 11 family + CPU 逐家注册 +
三处人工对齐* 变为 *include 公共 chunk + 一份绑定声明*。这实际拿到了该文档中暂缓的
P4（代码生成）的大部分收益，而代价只是一个 40 行解析器、KB 级显存冗余和一次逐
family 的机械迁移。
