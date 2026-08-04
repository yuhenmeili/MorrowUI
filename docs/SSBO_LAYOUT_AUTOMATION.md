# SSBO Layout 注册与属性填充优化方案

## 1. 背景

当前 `SSBOManager::registerSSBOLayout()` 根据 shader 名称，通过一组 `if / else if` 分支完成以下工作：

1. 选择 CPU 侧实例数据结构；
2. 设置每个实例的 `elementSize`；
3. 从 `Transform` 和 `Material` 读取数据；
4. 将业务参数打包到 `attr1`、`attr2`、`attr3`；
5. 创建对应的 `filler`。

示例：

```cpp
if (layout.name == "button") {
    layout.elementSize = sizeof(DefaultBatchData3Attr);
    layout.filler = [](void* data, const RenderBatch& batch, int index) {
        auto* instanceData = static_cast<DefaultBatchData3Attr*>(data);
        instanceData->model = batch.transforms[index]->getWorldMatrix();
        instanceData->attr1 =
            std::get<Vector4>(batch.materials[index]->getVector("color"));

        const auto size =
            std::get<Vector3>(batch.materials[index]->getVector("displaySize"));
        instanceData->attr2 = Vector4(
            size.x,
            size.y,
            batch.materials[index]->getFloat("rounding"),
            batch.materials[index]->getFloat("alpha"));

        instanceData->attr3 = Vector4(
            batch.materials[index]->getFloat("useTexture"),
            0.0f,
            0.0f,
            0.0f);
    };
}
```

随着支持 SSBO 的 shader 增加，该函数会持续膨胀，并且 CPU 结构、GLSL 结构和 Material 参数之间依靠人工保持一致。

本文只描述建议方案，暂不修改现有代码。

---

## 2. 自动化问题的边界

SSBO 自动化需要区分两个不同问题。

### 2.1 内存布局自动化

内存布局包括：

- 字段类型；
- 字段 offset；
- matrix stride；
- array stride；
- SSBO 实例的 element stride；
- CPU 数据结构与 GLSL `std430` 布局是否一致。

这部分可以通过以下方式自动化：

- C++ `sizeof` / `offsetof`；
- 编译期 `static_assert`；
- OpenGL shader reflection；
- schema/code generation。

### 2.2 业务数据来源自动化

业务数据来源包括：

```text
model       <- Transform::getWorldMatrix()
color       <- Material::getVector("color")
alpha       <- Material::getFloat("alpha")
rounding    <- Material::getFloat("rounding")
```

以及更复杂的分量打包：

```text
attr2.x <- displaySize.x
attr2.y <- displaySize.y
attr2.z <- rounding
attr2.w <- alpha
```

shader reflection 只能得知 `attr2` 是 `vec4` 以及它的内存位置，无法推断四个分量应该从哪些 Material 参数读取。

因此不建议追求“完全自动填充”。更合理的目标是：

> 布局自动化、数据来源声明化、填充过程类型安全化。

---

## 3. 当前实现存在的问题

### 3.1 `if / else` 注册链持续膨胀

新增 SSBO shader 时必须修改 `SSBOManager.cpp`：

```cpp
if (layout.name == "default_color") {
    // ...
} else if (layout.name == "default_image") {
    // ...
} else if (layout.name == "button") {
    // ...
}
```

存在以下问题：

- shader 名称与 SSBOManager 强耦合；
- 新增或删除 shader 容易遗漏注册；
- 单个函数同时处理注册、布局和业务打包；
- filler 难以独立测试；
- 未注册 shader 缺少明确的失败路径。

### 3.2 未知 shader 的处理不安全

当前流程会先创建空 layout：

```cpp
SSBOLayout layout = {
    .name = shaderName
};
registerSSBOLayout(layout);
```

如果 shader 没有命中任何注册分支：

- `elementSize` 可能没有有效值；
- `filler` 可能为空；
- 后续可能使用错误的 buffer size；
- 调用空 `std::function` 可能导致异常。

无论最终采用哪种自动化方案，都应优先增加显式校验：

```cpp
if (layout.elementSize == 0 || !layout.filler) {
    LOG_E("No valid SSBO layout for shader '{}'", shaderName);
    return;
}
```

同时将成员初始化为安全状态：

```cpp
struct SSBOLayout {
    std::string name;
    size_t elementSize = 0;
    SSBOFiller filler;
};
```

### 3.3 CPU 与 GLSL 布局依靠隐式约定

CPU 侧：

```cpp
struct DefaultBatchData2Attr {
    Matrix4 model;
    Vector4 attr1;
    Vector4 attr2;
};
```

GLSL 侧：

```glsl
struct InstanceData {
    mat4 model;
    vec4 displaySize;
    vec4 imageAttr;
};
```

当前依赖以下事实：

```text
mat4 = 64 bytes
vec4 = 16 bytes
```

但代码没有验证：

- `Matrix4` 的实际大小；
- `Vector4` 的实际大小；
- struct field offset；
- `std430` array stride；
- 编译器 padding 是否符合预期。

### 3.4 `attr1` / `attr2` / `attr3` 缺少业务语义

同一个结构在不同 shader 中有完全不同的意义：

| Shader | `attr1` | `attr2` | `attr3` |
|---|---|---|---|
| `default_color` | color | alpha | - |
| `image_normal` | displaySize | rounding + alpha | - |
| `font` | fontColor | alpha | - |
| `bounce` | meshCenter + alpha | 动画参数 | - |
| `button` | bgColor | displaySize + rounding + alpha | useTexture |

`DefaultBatchDataNAttr` 仅描述存储形状，不描述字段语义，容易造成 CPU 与 GLSL 字段顺序错位。

### 3.5 Material 读取缺少类型安全

当前代码包含：

```cpp
std::get<Vector4>(material->getVector("color"));
```

如果参数不存在，或者实际类型不是 `Vector4`，可能抛出 `std::bad_variant_access`。自动填充方案需要明确：

- 缺失参数是否报错；
- 是否允许默认值；
- 类型不匹配时如何处理；
- 是否只在开发版本执行严格校验。

---

## 4. 不建议直接采用的方案

### 4.1 仅依赖 C++ 自动反射

C++17 没有标准结构体反射能力，无法直接从：

```cpp
struct ButtonInstanceData {
    Matrix4 model;
    Vector4 color;
    Vector4 imageAttr;
};
```

自动枚举：

- 字段名称；
- 字段类型；
- 字段 offset；
- 字段对应的 Material 参数。

除非引入：

- 自研宏反射；
- Boost.Describe；
- RTTR；
- clang tooling；
- schema/code generation。

对于当前项目规模，直接引入完整 C++ 反射框架收益有限。

### 4.2 仅依赖 shader reflection

OpenGL shader reflection 可以查询：

- SSBO block；
- buffer variable；
- GLSL 类型；
- offset；
- array stride；
- matrix stride；
- top-level array stride。

但无法推断：

```text
imageAttr.x <- rounding
imageAttr.y <- alpha
```

因此 shader reflection 适合做布局发现和校验，不适合单独承担业务属性绑定。

### 4.3 每帧解析 shader 源码

不建议在运行时每帧解析 GLSL `struct InstanceData`：

- 增加不必要的 CPU 开销；
- 预处理宏和 include 会使解析变复杂；
- GLSL 源码声明不包含业务数据来源；
- 自研 GLSL parser 成本较高且容易不完整。

如果需要解析或反射，应在 shader 链接后执行一次并缓存。

---

## 5. 推荐总体架构

建议采用三层结构：

```text
Shader/GLSL layout
        ↓
SSBO layout descriptor
        ↓
typed field binding / custom packer
        ↓
instance buffer writer
```

具体原则：

1. shader 名称通过注册表寻找 layout；
2. layout 描述 element size 和字段；
3. 普通字段使用声明式 binding；
4. 复杂分量打包允许使用 custom packer；
5. shader reflection 用于校验 offset/type/stride；
6. 不在每帧进行字符串解析或动态注册；
7. 未注册或不匹配时明确失败，不使用无效 layout。

---

## 6. 阶段一：注册表 + 类型化 filler

这是最推荐的第一阶段，改动小、风险低。

### 6.1 将 `if / else` 改为注册表

示意：

```cpp
using SSBOLayoutFactory = std::function<SSBOLayout()>;

std::unordered_map<std::string, SSBOLayoutFactory> m_layoutFactories;
```

初始化：

```cpp
m_layoutFactories.emplace("default_color", [] {
    return makeLayout<DefaultBatchData2Attr>(
        "default_color",
        fillDefaultColor);
});

m_layoutFactories.emplace("default_image", [] {
    return makeLayout<DefaultBatchData1Attr>(
        "default_image",
        fillDefaultImage);
});
```

别名 shader 可以显式共享同一个 factory：

```cpp
const auto imageLayoutFactory = [] {
    return makeLayout<DefaultBatchData2Attr>(
        "image",
        fillImage);
};

m_layoutFactories.emplace("image_normal", imageLayoutFactory);
m_layoutFactories.emplace("image_text_debug", imageLayoutFactory);
m_layoutFactories.emplace("image_oes", imageLayoutFactory);
```

### 6.2 filler 拆成独立命名函数

```cpp
static void fillDefaultColor(
    void* destination,
    const RenderBatch& batch,
    size_t index);

static void fillButton(
    void* destination,
    const RenderBatch& batch,
    size_t index);
```

收益：

- 避免单个注册函数持续膨胀；
- filler 可以单独进行单元测试；
- shader 注册关系一目了然；
- 未注册 shader 可显式报错；
- 后续可以逐个迁移到字段 binding。

### 6.3 使用模板自动设置 element size

```cpp
template<typename T, typename Filler>
SSBOLayout makeLayout(
    std::string name,
    Filler&& filler) {
    static_assert(std::is_trivially_copyable_v<T>);

    SSBOLayout layout;
    layout.name = std::move(name);
    layout.elementSize = sizeof(T);
    layout.filler = std::forward<Filler>(filler);
    return layout;
}
```

这样可以避免手工重复填写：

```cpp
layout.elementSize = sizeof(...);
```

---

## 7. 阶段二：字段绑定描述

在注册表稳定后，可以将简单 filler 改为字段描述。

### 7.1 字段来源类型

```cpp
enum class SSBOValueSource {
    TransformWorldMatrix,
    MaterialFloat,
    MaterialVector2,
    MaterialVector3,
    MaterialVector4,
    MaterialVectorComponent,
    ConstantFloat,
    ConstantVector4,
    Custom
};
```

### 7.2 字段描述

```cpp
struct SSBOFieldBinding {
    std::string shaderField;
    size_t offset = 0;
    ShaderDataType type = ShaderDataType::Float;
    SSBOValueSource source = SSBOValueSource::Custom;
    std::string materialProperty;
    int sourceComponent = -1;
};
```

`default_image` 可以声明为：

```cpp
layout.fields = {
    makeWorldMatrixField(
        "model",
        offsetof(DefaultBatchData1Attr, model)),

    makePackedVector4Field(
        "defaultAttr",
        offsetof(DefaultBatchData1Attr, attr),
        {
            materialFloat("alpha"),
            constantFloat(0.0f),
            constantFloat(0.0f),
            constantFloat(0.0f)
        })
};
```

### 7.3 支持 vec4 分量打包

需要支持以下声明：

```cpp
makePackedVector4Field(
    "imageAttr",
    offsetof(ButtonInstanceData, imageAttr),
    {
        materialVectorComponent("displaySize", 0),
        materialVectorComponent("displaySize", 1),
        materialFloat("rounding"),
        materialFloat("alpha")
    });
```

通用 writer 根据 binding 将值写入目标地址。

### 7.4 保留 custom packer

并非所有业务数据都适合通用 binding。对于复杂逻辑，应允许：

```cpp
layout.customFiller = fillComplexEffect;
```

推荐原则：

- 简单的 property copy 使用 binding；
- 多字段计算或条件逻辑使用 custom filler；
- 不为了追求 100% 声明式而制造复杂 DSL。

---

## 8. 阶段三：OpenGL shader reflection 校验

项目当前 GL headers 已提供 program interface query 相关 API，可在 shader program 链接完成后查询 SSBO layout。

### 8.1 可查询的信息

通过：

```cpp
glGetProgramResourceIndex
glGetProgramResourceName
glGetProgramResourceiv
```

查询：

```cpp
GL_BUFFER_VARIABLE
GL_SHADER_STORAGE_BLOCK
GL_TYPE
GL_OFFSET
GL_ARRAY_STRIDE
GL_MATRIX_STRIDE
GL_TOP_LEVEL_ARRAY_STRIDE
GL_BLOCK_INDEX
```

可得到：

```text
model offset
color offset
imageAttr offset
InstanceData array stride
字段 GLSL 类型
```

### 8.2 推荐用途

shader reflection 主要用于开发期校验：

```text
CPU binding offset == GLSL reflected offset
CPU field type == GLSL field type
CPU elementSize == GLSL top-level array stride
```

如果不一致，应输出明确错误：

```text
SSBO layout mismatch:
shader=button
field=imageAttr
cpuOffset=80
shaderOffset=96
```

### 8.3 不建议每帧执行

reflection 应在以下时机执行一次：

- shader program 首次链接成功；
- shader hot reload；
- layout 首次注册。

结果缓存到 shader 或 layout 对象中。

---

## 9. 阶段四：统一 schema / 代码生成

如果未来 SSBO shader 数量明显增加，可以使用单一 schema 生成：

- GLSL `InstanceData`；
- C++ instance data struct；
- layout descriptor；
- field binding；
- static assertions；
- reflection validation metadata。

示例 schema：

```yaml
shader: button
fields:
  - name: model
    type: mat4
    source: transform.world

  - name: bgColor
    type: vec4
    source: material.color

  - name: imageAttr
    type: vec4
    components:
      x: material.displaySize.x
      y: material.displaySize.y
      z: material.rounding
      w: material.alpha

  - name: textureAttr
    type: vec4
    components:
      x: material.useTexture
      y: 0
      z: 0
      w: 0
```

生成物可以包括：

```text
ButtonInstanceData.generated.h
ButtonInstanceData.generated.cpp
button_instance_data.generated.glsl
```

这一阶段工具链成本较高，当前不建议优先实施。

---

## 10. CPU 数据结构建议

### 10.1 用语义化名称替代 attrN

不建议长期保留：

```cpp
DefaultBatchData2Attr {
    Matrix4 model;
    Vector4 attr1;
    Vector4 attr2;
};
```

建议逐步改为：

```cpp
struct ImageInstanceData {
    Matrix4 model;
    Vector4 displaySize;
    Vector4 imageAttr;
};

struct ButtonInstanceData {
    Matrix4 model;
    Vector4 backgroundColor;
    Vector4 imageAttr;
    Vector4 textureAttr;
};
```

这样 CPU 字段和 GLSL 字段可以使用一致名称，降低顺序错位和语义误解风险。

### 10.2 增加编译期布局检查

最低限度建议：

```cpp
static_assert(sizeof(Matrix4) == 64);
static_assert(sizeof(Vector4) == 16);

static_assert(offsetof(ButtonInstanceData, model) == 0);
static_assert(offsetof(ButtonInstanceData, backgroundColor) == 64);
static_assert(offsetof(ButtonInstanceData, imageAttr) == 80);
static_assert(offsetof(ButtonInstanceData, textureAttr) == 96);
static_assert(sizeof(ButtonInstanceData) == 112);
```

还应保证实例结构适合直接写入字节 buffer：

```cpp
static_assert(std::is_standard_layout_v<ButtonInstanceData>);
static_assert(std::is_trivially_copyable_v<ButtonInstanceData>);
```

---

## 11. Material 参数访问建议

自动 binding 依赖 Material 参数读取，因此应补充类型安全接口。

当前：

```cpp
std::get<Vector4>(material.getVector("color"));
```

建议提供：

```cpp
bool tryGetFloat(
    std::string_view name,
    float& value) const;

bool tryGetVector4(
    std::string_view name,
    Vector4& value) const;

float getFloatOr(
    std::string_view name,
    float defaultValue) const;

Vector4 getVector4Or(
    std::string_view name,
    const Vector4& defaultValue) const;
```

字段 binding 可声明参数是否必需：

```cpp
struct MaterialPropertyBinding {
    std::string name;
    bool required = true;
    Vector4 defaultValue = Vector4::ZERO;
};
```

开发版本中：

- 缺少 required 参数时输出错误；
- 类型不匹配时输出 shader、字段和 Material 参数名；
- 避免 `std::bad_variant_access`。

---

## 12. 推荐接口草案

```cpp
using SSBOFiller =
    std::function<bool(
        void* destination,
        const RenderBatch& batch,
        size_t index)>;

struct SSBOLayout {
    std::string shaderName;
    size_t elementSize = 0;
    std::vector<SSBOFieldBinding> fields;
    SSBOFiller filler;

    bool isValid() const {
        return elementSize > 0 && (filler || !fields.empty());
    }
};
```

管理器：

```cpp
class SSBOManager {
public:
    bool registerLayout(SSBOLayout layout);
    bool unregisterLayout(std::string_view shaderName);

    bool updateSSBOForShader(
        std::string_view shaderName,
        RenderBatch& batch);

private:
    const SSBOLayout* findLayout(
        std::string_view shaderName) const;

    bool fillInstance(
        const SSBOLayout& layout,
        void* destination,
        const RenderBatch& batch,
        size_t index) const;

    std::unordered_map<std::string, SSBOLayout> m_layouts;
};
```

模板注册辅助：

```cpp
template<typename T, typename Filler>
bool registerTypedLayout(
    std::string shaderName,
    Filler&& filler) {
    static_assert(std::is_standard_layout_v<T>);
    static_assert(std::is_trivially_copyable_v<T>);

    SSBOLayout layout;
    layout.shaderName = std::move(shaderName);
    layout.elementSize = sizeof(T);
    layout.filler = std::forward<Filler>(filler);
    return registerLayout(std::move(layout));
}
```

---

## 13. 错误处理建议

以下情况必须显式失败：

1. shader 声明支持 SSBO，但没有注册 layout；
2. `elementSize == 0`；
3. filler 为空且 fields 为空；
4. `materials.size()`、`transforms.size()` 与 batch size 不一致；
5. Material required 参数不存在；
6. Material 参数类型不匹配；
7. shader reflection 与 CPU layout 不一致；
8. `field.offset + field.size > elementSize`。

推荐 `updateSSBOForShader()` 返回 `bool`：

```cpp
if (!ssboManager->updateSSBOForShader(batch.shaderName, batch)) {
    // 使用安全 fallback 或跳过该 batch
}
```

不应在 layout 无效时继续 resize 和写入。

---

## 14. 测试建议

### 14.1 纯 CPU 单元测试

对每种 filler 测试：

- model matrix 是否写入正确；
- Material 参数是否写入正确分量；
- 默认值是否正确；
- 参数缺失是否按约定失败；
- element size 是否正确；
- 多实例写入是否互不覆盖。

### 14.2 布局测试

验证：

```cpp
sizeof(...)
offsetof(...)
is_standard_layout
is_trivially_copyable
```

### 14.3 shader reflection 集成测试

在支持 OpenGL 4.3+ 的测试环境中：

- 编译 SSBO shader；
- 反射 `InstanceData`；
- 对比字段 offset/type/stride；
- 对不一致 shader 给出可诊断错误。

### 14.4 未注册 shader 测试

验证未知 shader：

- 不分配异常大小 buffer；
- 不调用空 filler；
- 输出一次明确错误；
- 能进入约定的 fallback。

---

## 15. 分阶段实施顺序

### P0：安全性修复

1. `SSBOLayout::elementSize` 默认初始化为 `0`；
2. 验证 `filler` 是否有效；
3. 未注册 shader 明确报错并停止 SSBO 更新；
4. 验证 batch 各数组长度；
5. 增加基础 `static_assert`。

### P1：注册表重构

1. 将 `if / else` 改为 shader → layout factory 注册表；
2. filler 拆为独立命名函数；
3. 模板自动设置 `sizeof(T)`；
4. 为 filler 增加纯 CPU 测试；
5. 合并共享 layout 的 shader 别名。

### P2：字段绑定

1. 引入 `SSBOFieldBinding`；
2. 支持 world matrix、Material float/vector 和常量；
3. 支持 vec4 component packing；
4. 简单 shader 迁移到声明式 binding；
5. 复杂 shader 保留 custom filler。

### P3：shader reflection 校验

1. shader 链接后反射 SSBO block；
2. 缓存字段 offset/type/stride；
3. 开发版本校验 CPU descriptor；
4. shader hot reload 时重新校验。

### P4：可选代码生成

仅当 SSBO shader 数量和维护成本继续增长时，再引入统一 schema 和代码生成。

---

## 16. 最终建议

当前项目最适合的方案不是一次性实现“完全自动属性填充”，而是：

```text
注册表
  + typed filler
  + 可选字段 binding
  + shader reflection 校验
```

建议优先实施 P0 和 P1：

- 解决未知 shader 的安全风险；
- 移除持续增长的 `if / else`；
- 保持当前业务行为不变；
- 为后续自动化建立稳定接口。

随后按实际重复程度实施 P2。P3 用于防止 CPU/GLSL layout 静默错位。P4 只在 shader 数量足够多时考虑。

核心结论：

> SSBO 的字段位置和类型可以自动发现或校验，但字段的业务数据来源无法仅凭 shader 自动推断。应自动化布局，将数据来源做成显式、类型安全、可测试的声明。
