//
// Created by 0060328 on 25-10-23.
//

#include "SSBOManager.h"

#include <type_traits>
#include <utility>

#include "GlobalObject.h"
#include "Log.h"
#include "Material.h"
#include "SSBOFieldBinding.h"
#include "ShaderReflection.h"
#include "core/BatchDataDefine.h"

namespace morrow {

// SSBO block 名（所有内建 shader 统一）
constexpr const char* kSSBOBlockName = "InstanceBuffer";

// ---------------------------------------------------------------------------
// P0：编译期布局检查，保证 CPU 结构尺寸与 GLSL std430 约定一致
// ---------------------------------------------------------------------------
static_assert(sizeof(Matrix4) == 64, "Matrix4 must be 64 bytes to match GLSL mat4");
static_assert(sizeof(Vector4) == 16, "Vector4 must be 16 bytes to match GLSL vec4");

static_assert(offsetof(DefaultBatchData1Attr, model) == 0, "model must be at offset 0");
static_assert(offsetof(DefaultBatchData1Attr, attr) == 64, "attr must be at offset 64");
static_assert(sizeof(DefaultBatchData1Attr) == 80, "DefaultBatchData1Attr must be 80 bytes");

static_assert(offsetof(DefaultBatchData2Attr, model) == 0, "model must be at offset 0");
static_assert(offsetof(DefaultBatchData2Attr, attr1) == 64, "attr1 must be at offset 64");
static_assert(offsetof(DefaultBatchData2Attr, attr2) == 80, "attr2 must be at offset 80");
static_assert(sizeof(DefaultBatchData2Attr) == 96, "DefaultBatchData2Attr must be 96 bytes");

static_assert(offsetof(DefaultBatchData3Attr, model) == 0, "model must be at offset 0");
static_assert(offsetof(DefaultBatchData3Attr, attr1) == 64, "attr1 must be at offset 64");
static_assert(offsetof(DefaultBatchData3Attr, attr2) == 80, "attr2 must be at offset 80");
static_assert(offsetof(DefaultBatchData3Attr, attr3) == 96, "attr3 must be at offset 96");
static_assert(sizeof(DefaultBatchData3Attr) == 112, "DefaultBatchData3Attr must be 112 bytes");

static_assert(std::is_standard_layout_v<DefaultBatchData1Attr>, "DefaultBatchData1Attr must be standard layout");
static_assert(std::is_standard_layout_v<DefaultBatchData2Attr>, "DefaultBatchData2Attr must be standard layout");
static_assert(std::is_standard_layout_v<DefaultBatchData3Attr>, "DefaultBatchData3Attr must be standard layout");

// 说明：未断言 is_trivially_copyable。Vector4 的用户自定义拷贝构造函数
// （memcpy 实现）使其不为 trivially copyable；标准布局 + 尺寸/偏移断言
// 已足以保证与 GLSL std430 布局一致。

// ---------------------------------------------------------------------------
// P1：模板注册辅助，自动设置 elementSize；fields / filler 由注册处填充
// ---------------------------------------------------------------------------
template <typename T>
SSBOLayout makeLayout(std::string name) {
    // Vector4 有用户自定义拷贝构造函数，故不要求 trivially copyable；
    // 标准布局保证字段 offset 由声明顺序确定，配合上方尺寸/偏移断言即可。
    static_assert(std::is_standard_layout_v<T>, "SSBO instance data must be standard layout");

    SSBOLayout layout;
    layout.name = std::move(name);
    layout.elementSize = sizeof(T);
    return layout;
}

// ---------------------------------------------------------------------------
// P1/P2：注册表初始化（替代原 if/else 注册链），
//        简单 shader 使用声明式字段绑定；别名 shader 共享 factory
// ---------------------------------------------------------------------------
SSBOManager::SSBOManager() {
    registerLayout("default_color", [] {
        auto layout = makeLayout<DefaultBatchData2Attr>("default_color");
        layout.fields = {
            makeWorldMatrixField("model", offsetof(DefaultBatchData2Attr, model)),
            makeMaterialVectorField("defaultColor", offsetof(DefaultBatchData2Attr, attr1), SSBOValueSource::MaterialVector4, "color"),
            makePackedVector4Field("defaultAttr", offsetof(DefaultBatchData2Attr, attr2), {materialFloat("alpha"), constantFloat(0.0f), constantFloat(0.0f), constantFloat(0.0f)})};
        return layout;
    });

    // 共享 DefaultBatchData1Attr 布局的 shader 别名
    const auto defaultImageLayoutFactory = [] {
        auto layout = makeLayout<DefaultBatchData1Attr>("default_image");
        layout.fields = {
            makeWorldMatrixField("model", offsetof(DefaultBatchData1Attr, model)),
            makePackedVector4Field("defaultAttr", offsetof(DefaultBatchData1Attr, attr), {materialFloat("alpha"), constantFloat(0.0f), constantFloat(0.0f), constantFloat(0.0f)})};
        return layout;
    };
    registerLayout("default_image", defaultImageLayoutFactory);
    registerLayout("anchor_point_scale", defaultImageLayoutFactory);

    // 共享 DefaultBatchData2Attr 布局的 shader 别名
    const auto imageLayoutFactory = [] {
        auto layout = makeLayout<DefaultBatchData2Attr>("image_normal");
        layout.fields = {makeWorldMatrixField("model", offsetof(DefaultBatchData2Attr, model)),
                         makePackedVector4Field("displaySize", offsetof(DefaultBatchData2Attr, attr1),
                                                {materialVectorComponent("displaySize", 0), materialVectorComponent("displaySize", 1), materialVectorComponent("displaySize", 2),
                                                 constantFloat(0.0f)}),
                         makePackedVector4Field("imageAttr", offsetof(DefaultBatchData2Attr, attr2),
                                                {materialFloat("rounding"), materialFloat("alpha"), constantFloat(0.0f), constantFloat(0.0f)})};
        return layout;
    };
    registerLayout("image_normal", imageLayoutFactory);
    registerLayout("image_text_debug", imageLayoutFactory);
    registerLayout("image_oes", imageLayoutFactory);

    registerLayout("font", [] {
        auto layout = makeLayout<DefaultBatchData2Attr>("font");
        layout.fields = {
            makeWorldMatrixField("model", offsetof(DefaultBatchData2Attr, model)),
            makeMaterialVectorField("fontColor", offsetof(DefaultBatchData2Attr, attr1), SSBOValueSource::MaterialVector4, "fontColor"),
            makePackedVector4Field("fontAttr", offsetof(DefaultBatchData2Attr, attr2), {materialFloat("alpha"), constantFloat(0.0f), constantFloat(0.0f), constantFloat(0.0f)})};
        return layout;
    });

    registerLayout("bounce", [] {
        auto layout = makeLayout<DefaultBatchData2Attr>("bounce");
        layout.fields = {makeWorldMatrixField("model", offsetof(DefaultBatchData2Attr, model)),
                         makePackedVector4Field("meshCenter", offsetof(DefaultBatchData2Attr, attr1),
                                                {materialVectorComponent("meshCenter", 0), materialVectorComponent("meshCenter", 1), materialVectorComponent("meshCenter", 2),
                                                 materialFloat("alpha")}),
                         makePackedVector4Field("defaultAttr", offsetof(DefaultBatchData2Attr, attr2),
                                                {materialFloat("timeDelta"), materialFloat("duration"), materialFloat("bounceTimes"), materialFloat("scaleRange")})};
        return layout;
    });

    registerLayout("button", [] {
        auto layout = makeLayout<DefaultBatchData3Attr>("button");
        layout.fields = {
            makeWorldMatrixField("model", offsetof(DefaultBatchData3Attr, model)),
            makeMaterialVectorField("bgColor", offsetof(DefaultBatchData3Attr, attr1), SSBOValueSource::MaterialVector4, "color"),
            makePackedVector4Field("defaultAttr", offsetof(DefaultBatchData3Attr, attr2),
                                   {materialVectorComponent("displaySize", 0), materialVectorComponent("displaySize", 1), materialFloat("rounding"), materialFloat("alpha")}),
            makePackedVector4Field("textureAttr", offsetof(DefaultBatchData3Attr, attr3),
                                   {materialFloat("useTexture"), constantFloat(0.0f), constantFloat(0.0f), constantFloat(0.0f)})};
        return layout;
    });
}

void SSBOManager::registerLayout(const std::string& shaderName, SSBOLayoutFactory factory) {
    m_layoutFactories[shaderName] = std::move(factory);
}

const SSBOLayout* SSBOManager::findLayout(const std::string& shaderName) const {
    auto cached = m_shaderLayouts.find(shaderName);
    if (cached != m_shaderLayouts.end()) {
        return &cached->second;
    }
    auto factory = m_layoutFactories.find(shaderName);
    if (factory == m_layoutFactories.end()) {
        return nullptr;
    }
    auto [it, inserted] = m_shaderLayouts.emplace(shaderName, factory->second());
    (void)inserted;
    return &it->second;
}

bool SSBOManager::hasLayout(const std::string& shaderName) const {
    return findLayout(shaderName) != nullptr;
}

const SSBOLayout* SSBOManager::getLayout(const std::string& shaderName) const {
    return findLayout(shaderName);
}

// ---------------------------------------------------------------------------
// P3：shader 首次使用时执行一次 SSBO reflection 校验并缓存结果
// ---------------------------------------------------------------------------
void SSBOManager::validateReflectionOnce(const std::string& shaderName, const SSBOLayout& layout, const RenderBatch& batch) {
    if (batch.materials.empty()) {
        return;
    }
    const HwGPUProgram program = batch.materials[0]->getBatchShader();
    if (!program.isValid()) {
        LOG_W("SSBO reflection skipped for shader '{}': batch shader invalid", shaderName);
        return;
    }

    // 已对同一 program 校验过则跳过；shader hot reload（新 program id）会重新校验
    auto cached = m_reflectionValidated.find(shaderName);
    if (cached != m_reflectionValidated.end() && cached->second == program.id) {
        return;
    }
    m_reflectionValidated[shaderName] = program.id;

    const auto reflected = RENDERINGTHREAD->reflectSSBOBlock(program, kSSBOBlockName);
    if (!reflected.valid) {
        LOG_W("SSBO reflection unavailable for shader '{}', skip layout validation", shaderName);
        return;
    }

    const std::string error = validateSSBOLayout(layout, reflected);
    if (!error.empty()) {
        LOG_E("SSBO layout mismatch for shader '{}': {}", shaderName, error);
    }
}

void SSBOManager::updateSSBOForShader(const std::string& shaderName, RenderBatch& batch) {
    // P0：SSBO 对象缺失时安全返回
    if (batch.ssbo == nullptr) {
        LOG_E("{} batch SSBO is null", shaderName);
        return;
    }

    // P0：未注册 shader 明确报错并停止 SSBO 更新
    const SSBOLayout* layout = findLayout(shaderName);
    if (layout == nullptr) {
        LOG_E("No SSBO layout registered for shader '{}', skip SSBO update", shaderName);
        return;
    }

    // P0：验证 layout 有效性（elementSize > 0 且 fields/filler 有效）
    if (!layout->isValid()) {
        LOG_E("Invalid SSBO layout for shader '{}': elementSize={}, fields={}, filler={}", shaderName, layout->elementSize, layout->fields.size(),
              static_cast<bool>(layout->filler));
        return;
    }

    // P0：验证 batch 各数组长度一致，避免填充时越界
    const auto batchSize = batch.materials.size();
    if (batch.transforms.size() != batchSize || batch.meshFilters.size() != batchSize) {
        LOG_E("Batch array length mismatch for shader '{}': materials={}, transforms={}, meshFilters={}", shaderName, batchSize, batch.transforms.size(), batch.meshFilters.size());
        return;
    }

    // P3：首次遇到该 shader 时反射校验 CPU/GLSL 布局一致性（结果缓存）
    validateReflectionOnce(shaderName, *layout, batch);

    // 准备SSBO数据缓冲区
    batch.ssbo->resize(batchSize * layout->elementSize);
    auto ptr = static_cast<uint8_t*>(batch.ssbo->getDataPtr());
    // 为每个实例填充数据（P2：声明式字段绑定 + custom filler）
    for (size_t i = 0; i < batchSize; i++) {
        void* instanceData = ptr + i * layout->elementSize;
        fillSSBOInstance(*layout, instanceData, batch, i);
    }
    // 更新SSBO到GPU
    batch.ssbo->update();
}

}  // namespace morrow
