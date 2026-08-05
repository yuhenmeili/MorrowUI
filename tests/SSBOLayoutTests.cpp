//
// P2/P3：SSBO 字段绑定 + reflection 校验 纯 CPU 单元测试
//
// 只验证 CPU 侧字段绑定填充（writeSSBOField / fillSSBOInstance）与
// layout 校验（validateSSBOLayout），不创建 GPU 资源，不经过
// updateSSBOForShader（其会触发 GPU 上传与反射查询）。
//

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "core/BatchDataDefine.h"
#include "renderer/resource/SSBOFieldBinding.h"
#include "renderer/resource/SSBOManager.h"
#include "renderer/resource/ShaderReflection.h"
#include "renderer/resource/Material.h"
#include "ui/base/Transform.h"

using namespace morrow;
using namespace morrow::Math;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

void expectVec4(const Vector4& actual, float x, float y, float z, float w, const std::string& message) {
    expect(actual.x == x && actual.y == y && actual.z == z && actual.w == w, message);
}

bool sameMatrix(const Matrix4& a, const Matrix4& b) {
    return std::memcmp(a.elements, b.elements, sizeof(float) * 16) == 0;
}

// 构建 count 个实例的 batch；第 i 个 Transform 位置为 (i, i*2, 0)
RenderBatch makeBatch(size_t count) {
    RenderBatch batch;
    for (size_t i = 0; i < count; i++) {
        auto material = Material::create();
        auto transform = std::make_shared<Transform>();
        transform->setPosition(static_cast<float>(i), static_cast<float>(i) * 2.0f, 0.0f);
        batch.materials.emplace_back(std::move(material));
        batch.transforms.emplace_back(std::move(transform));
        batch.meshFilters.emplace_back(nullptr);
    }
    return batch;
}

// ════════════════════════════════════════════════════════════════════
// P2：字段绑定 writer
// ════════════════════════════════════════════════════════════════════

void testWriteWorldMatrixField() {
    auto batch = makeBatch(1);
    const auto field = makeWorldMatrixField("model", 0);
    uint8_t buffer[sizeof(Matrix4)]{};
    expect(writeSSBOField(field, buffer, batch, 0), "world matrix field should write");
    const auto* model = reinterpret_cast<const Matrix4*>(buffer);
    expect(sameMatrix(*model, batch.transforms[0]->getWorldMatrix()),
           "world matrix field should match transform world matrix");
}

void testWriteMaterialVectorField() {
    auto batch = makeBatch(1);
    batch.materials[0]->setVector("color", Vector4(1.0f, 2.0f, 3.0f, 4.0f));
    const auto field = makeMaterialVectorField(
        "defaultColor", 0, SSBOValueSource::MaterialVector4, "color");
    uint8_t buffer[sizeof(Vector4)]{};
    expect(writeSSBOField(field, buffer, batch, 0), "material vector field should write");
    const auto* value = reinterpret_cast<const Vector4*>(buffer);
    expectVec4(*value, 1.0f, 2.0f, 3.0f, 4.0f, "material vector4 should be written");
}

void testWritePackedVector4Field() {
    auto batch = makeBatch(1);
    batch.materials[0]->setVector("displaySize", Vector3(10.0f, 20.0f, 30.0f));
    batch.materials[0]->setFloat("rounding", 0.3f);
    const auto field = makePackedVector4Field("imageAttr", 0, {
        materialVectorComponent("displaySize", 0),
        materialVectorComponent("displaySize", 1),
        materialFloat("rounding"),
        constantFloat(0.5f)
    });
    uint8_t buffer[sizeof(Vector4)]{};
    expect(writeSSBOField(field, buffer, batch, 0), "packed vec4 field should write");
    const auto* value = reinterpret_cast<const Vector4*>(buffer);
    expectVec4(*value, 10.0f, 20.0f, 0.3f, 0.5f, "packed vec4 components should match");
}

// ════════════════════════════════════════════════════════════════════
// P2：各 shader 注册表 layout 填充（验证字段声明与填充一致性）
// ════════════════════════════════════════════════════════════════════

void testDefaultColorLayout() {
    auto batch = makeBatch(1);
    batch.materials[0]->setVector("color", Vector4(1.0f, 2.0f, 3.0f, 4.0f));
    batch.materials[0]->setFloat("alpha", 0.5f);

    SSBOManager manager;
    const auto* layout = manager.getLayout("default_color");
    expect(layout != nullptr, "default_color layout should be registered");
    if (!layout) return;
    std::vector<uint8_t> buffer(layout->elementSize);
    fillSSBOInstance(*layout, buffer.data(), batch, 0);

    const auto* data = reinterpret_cast<const DefaultBatchData2Attr*>(buffer.data());
    expect(sameMatrix(data->model, batch.transforms[0]->getWorldMatrix()),
           "default_color: model matrix should match");
    expectVec4(data->attr1, 1.0f, 2.0f, 3.0f, 4.0f, "default_color: attr1 should be color");
    expectVec4(data->attr2, 0.5f, 0.0f, 0.0f, 0.0f, "default_color: attr2.x should be alpha");
}

void testDefaultImageLayout() {
    auto batch = makeBatch(1);
    batch.materials[0]->setFloat("alpha", 0.25f);

    SSBOManager manager;
    const auto* layout = manager.getLayout("default_image");
    expect(layout != nullptr, "default_image layout should be registered");
    if (!layout) return;
    std::vector<uint8_t> buffer(layout->elementSize);
    fillSSBOInstance(*layout, buffer.data(), batch, 0);

    const auto* data = reinterpret_cast<const DefaultBatchData1Attr*>(buffer.data());
    expect(sameMatrix(data->model, batch.transforms[0]->getWorldMatrix()),
           "default_image: model matrix should match");
    expectVec4(data->attr, 0.25f, 0.0f, 0.0f, 0.0f, "default_image: attr.x should be alpha");
}

void testImageNormalLayout() {
    auto batch = makeBatch(1);
    batch.materials[0]->setVector("displaySize", Vector3(10.0f, 20.0f, 30.0f));
    batch.materials[0]->setFloat("rounding", 0.3f);
    batch.materials[0]->setFloat("alpha", 0.6f);

    SSBOManager manager;
    const auto* layout = manager.getLayout("image_normal");
    expect(layout != nullptr, "image_normal layout should be registered");
    if (!layout) return;
    std::vector<uint8_t> buffer(layout->elementSize);
    fillSSBOInstance(*layout, buffer.data(), batch, 0);

    const auto* data = reinterpret_cast<const DefaultBatchData2Attr*>(buffer.data());
    expect(sameMatrix(data->model, batch.transforms[0]->getWorldMatrix()),
           "image_normal: model matrix should match");
    expectVec4(data->attr1, 10.0f, 20.0f, 30.0f, 0.0f, "image_normal: attr1 should be displaySize");
    expectVec4(data->attr2, 0.3f, 0.6f, 0.0f, 0.0f, "image_normal: attr2 should be (rounding, alpha)");
}

void testFontLayout() {
    auto batch = makeBatch(1);
    batch.materials[0]->setVector("fontColor", Vector4(1.0f, 0.0f, 0.0f, 1.0f));
    batch.materials[0]->setFloat("alpha", 0.8f);

    SSBOManager manager;
    const auto* layout = manager.getLayout("font");
    expect(layout != nullptr, "font layout should be registered");
    if (!layout) return;
    std::vector<uint8_t> buffer(layout->elementSize);
    fillSSBOInstance(*layout, buffer.data(), batch, 0);

    const auto* data = reinterpret_cast<const DefaultBatchData2Attr*>(buffer.data());
    expect(sameMatrix(data->model, batch.transforms[0]->getWorldMatrix()),
           "font: model matrix should match");
    expectVec4(data->attr1, 1.0f, 0.0f, 0.0f, 1.0f, "font: attr1 should be fontColor");
    expectVec4(data->attr2, 0.8f, 0.0f, 0.0f, 0.0f, "font: attr2.x should be alpha");
}

void testBounceLayout() {
    auto batch = makeBatch(1);
    batch.materials[0]->setVector("meshCenter", Vector3(5.0f, 6.0f, 7.0f));
    batch.materials[0]->setFloat("alpha", 0.9f);
    batch.materials[0]->setFloat("timeDelta", 1.0f);
    batch.materials[0]->setFloat("duration", 2.0f);
    batch.materials[0]->setFloat("bounceTimes", 3.0f);
    batch.materials[0]->setFloat("scaleRange", 4.0f);

    SSBOManager manager;
    const auto* layout = manager.getLayout("bounce");
    expect(layout != nullptr, "bounce layout should be registered");
    if (!layout) return;
    std::vector<uint8_t> buffer(layout->elementSize);
    fillSSBOInstance(*layout, buffer.data(), batch, 0);

    const auto* data = reinterpret_cast<const DefaultBatchData2Attr*>(buffer.data());
    expect(sameMatrix(data->model, batch.transforms[0]->getWorldMatrix()),
           "bounce: model matrix should match");
    expectVec4(data->attr1, 5.0f, 6.0f, 7.0f, 0.9f, "bounce: attr1 should be (meshCenter, alpha)");
    expectVec4(data->attr2, 1.0f, 2.0f, 3.0f, 4.0f, "bounce: attr2 should be animation params");
}

void testButtonLayout() {
    auto batch = makeBatch(1);
    batch.materials[0]->setVector("color", Vector4(0.0f, 1.0f, 0.0f, 1.0f));
    batch.materials[0]->setVector("displaySize", Vector3(100.0f, 200.0f, 0.0f));
    batch.materials[0]->setFloat("rounding", 0.15f);
    batch.materials[0]->setFloat("alpha", 0.7f);
    batch.materials[0]->setFloat("useTexture", 1.0f);

    SSBOManager manager;
    const auto* layout = manager.getLayout("button");
    expect(layout != nullptr, "button layout should be registered");
    if (!layout) return;
    std::vector<uint8_t> buffer(layout->elementSize);
    fillSSBOInstance(*layout, buffer.data(), batch, 0);

    const auto* data = reinterpret_cast<const DefaultBatchData3Attr*>(buffer.data());
    expect(sameMatrix(data->model, batch.transforms[0]->getWorldMatrix()),
           "button: model matrix should match");
    expectVec4(data->attr1, 0.0f, 1.0f, 0.0f, 1.0f, "button: attr1 should be color");
    expectVec4(data->attr2, 100.0f, 200.0f, 0.15f, 0.7f,
               "button: attr2 should be (displaySize, rounding, alpha)");
    expectVec4(data->attr3, 1.0f, 0.0f, 0.0f, 0.0f, "button: attr3.x should be useTexture");
}

// 别名 shader 共享同一 factory：布局内容一致（各别名独立缓存实例，地址不同）
void testShaderAliasesShareLayout() {
    SSBOManager manager;
    const auto* a = manager.getLayout("image_normal");
    const auto* b = manager.getLayout("image_text_debug");
    const auto* c = manager.getLayout("image_oes");
    expect(a != nullptr && b != nullptr && c != nullptr,
           "image shader aliases should be registered");
    if (!a || !b || !c) return;

    expect(a->elementSize == b->elementSize && b->elementSize == c->elementSize,
           "image shader aliases should share element size");
    expect(a->fields.size() == b->fields.size() && b->fields.size() == c->fields.size(),
           "image shader aliases should share field count");

    bool sameFields = true;
    for (size_t i = 0; i < a->fields.size(); ++i) {
        if (a->fields[i].shaderField != b->fields[i].shaderField ||
            b->fields[i].shaderField != c->fields[i].shaderField ||
            a->fields[i].offset != b->fields[i].offset) {
            sameFields = false;
            break;
        }
    }
    expect(sameFields, "image shader aliases should share field definitions");
}

// ════════════════════════════════════════════════════════════════════
// P2：缺失参数安全回退（类型安全接口，不再抛 std::bad_variant_access）
// ════════════════════════════════════════════════════════════════════

void testMissingParamsFallbackSafely() {
    auto batch = makeBatch(1);  // 未设置任何 Material 参数

    SSBOManager manager;

    // image_normal：displaySize / rounding / alpha 全部缺失
    const auto* imageLayout = manager.getLayout("image_normal");
    std::vector<uint8_t> imageBuffer(imageLayout->elementSize);
    fillSSBOInstance(*imageLayout, imageBuffer.data(), batch, 0);  // 不应抛异常
    const auto* imageData = reinterpret_cast<const DefaultBatchData2Attr*>(imageBuffer.data());
    expectVec4(imageData->attr1, 0.0f, 0.0f, 0.0f, 0.0f,
               "image_normal: missing displaySize should fall back to zero");
    expectVec4(imageData->attr2, 0.0f, 0.0f, 0.0f, 0.0f,
               "image_normal: missing rounding/alpha should fall back to zero");

    // button：color / displaySize / useTexture 全部缺失
    const auto* buttonLayout = manager.getLayout("button");
    std::vector<uint8_t> buttonBuffer(buttonLayout->elementSize);
    fillSSBOInstance(*buttonLayout, buttonBuffer.data(), batch, 0);  // 不应抛异常
    const auto* buttonData = reinterpret_cast<const DefaultBatchData3Attr*>(buttonBuffer.data());
    expectVec4(buttonData->attr1, 0.0f, 0.0f, 0.0f, 0.0f,
               "button: missing color should fall back to zero");
    expectVec4(buttonData->attr3, 0.0f, 0.0f, 0.0f, 0.0f,
               "button: missing useTexture should fall back to zero");
}

// ════════════════════════════════════════════════════════════════════
// P2：多实例写入互不覆盖
// ════════════════════════════════════════════════════════════════════

void testMultipleInstancesDoNotOverlap() {
    constexpr size_t count = 3;
    auto batch = makeBatch(count);
    for (size_t i = 0; i < count; i++) {
        batch.materials[i]->setFloat("alpha", static_cast<float>(i) + 1.0f);
    }

    SSBOManager manager;
    const auto* layout = manager.getLayout("default_color");
    std::vector<uint8_t> buffer(count * layout->elementSize, 0xAA);
    for (size_t i = 0; i < count; i++) {
        fillSSBOInstance(*layout, buffer.data() + i * layout->elementSize, batch, i);
    }

    for (size_t i = 0; i < count; i++) {
        const auto* instance = reinterpret_cast<const DefaultBatchData2Attr*>(
            buffer.data() + i * layout->elementSize);
        expect(sameMatrix(instance->model, batch.transforms[i]->getWorldMatrix()),
               "multi-instance: each model matrix should match its own transform");
        expectVec4(instance->attr2, static_cast<float>(i) + 1.0f, 0.0f, 0.0f, 0.0f,
                   "multi-instance: each alpha should be independent");
    }
}

// ════════════════════════════════════════════════════════════════════
// P3：validateSSBOLayout 纯 CPU 校验
// ════════════════════════════════════════════════════════════════════

SSBOReflectedLayout makeMatchingGlslLayout() {
    SSBOReflectedLayout glsl;
    glsl.valid = true;
    glsl.topLevelArrayStride = sizeof(DefaultBatchData2Attr);
    glsl.fields = {
        {"model", ShaderDataType::Matrix4, 0},
        {"defaultColor", ShaderDataType::Vector4, 64},
        {"defaultAttr", ShaderDataType::Vector4, 80},
    };
    return glsl;
}

void testValidateLayoutMatches() {
    SSBOManager manager;
    const auto* cpu = manager.getLayout("default_color");
    expect(validateSSBOLayout(*cpu, makeMatchingGlslLayout()).empty(),
           "matching CPU/GLSL layout should pass validation");
}

void testValidateOffsetMismatch() {
    SSBOManager manager;
    const auto* cpu = manager.getLayout("default_color");
    auto glsl = makeMatchingGlslLayout();
    glsl.fields[1].offset = 63;
    const std::string error = validateSSBOLayout(*cpu, glsl);
    expect(!error.empty() && error.find("defaultColor") != std::string::npos,
           "offset mismatch should report the field name");
}

void testValidateTypeMismatch() {
    SSBOManager manager;
    const auto* cpu = manager.getLayout("default_color");
    auto glsl = makeMatchingGlslLayout();
    glsl.fields[0].type = ShaderDataType::Vector4;  // model 应为 mat4
    expect(!validateSSBOLayout(*cpu, glsl).empty(),
           "type mismatch should fail validation");
}

void testValidateMissingField() {
    SSBOManager manager;
    const auto* cpu = manager.getLayout("default_color");
    auto glsl = makeMatchingGlslLayout();
    glsl.fields.pop_back();  // 缺少 defaultAttr
    const std::string error = validateSSBOLayout(*cpu, glsl);
    expect(!error.empty() && error.find("defaultAttr") != std::string::npos,
           "missing GLSL field should report the field name");
}

void testValidateStrideMismatch() {
    SSBOManager manager;
    const auto* cpu = manager.getLayout("default_color");
    auto glsl = makeMatchingGlslLayout();
    glsl.topLevelArrayStride = 80;  // 与 elementSize 96 不一致
    expect(!validateSSBOLayout(*cpu, glsl).empty(),
           "element stride mismatch should fail validation");
}

void testValidateUnavailableReflection() {
    SSBOManager manager;
    const auto* cpu = manager.getLayout("default_color");
    SSBOReflectedLayout glsl;  // valid = false
    expect(validateSSBOLayout(*cpu, glsl).empty(),
           "unavailable reflection should not fail validation");
}

// ════════════════════════════════════════════════════════════════════
// 布局尺寸
// ════════════════════════════════════════════════════════════════════

void testElementSizesMatchStructs() {
    expect(sizeof(DefaultBatchData1Attr) == 80, "DefaultBatchData1Attr must be 80 bytes");
    expect(sizeof(DefaultBatchData2Attr) == 96, "DefaultBatchData2Attr must be 96 bytes");
    expect(sizeof(DefaultBatchData3Attr) == 112, "DefaultBatchData3Attr must be 112 bytes");
    expect(shaderDataTypeSize(ShaderDataType::Matrix4) == 64, "mat4 must be 64 bytes");
    expect(shaderDataTypeSize(ShaderDataType::Vector4) == 16, "vec4 must be 16 bytes");
}

} // namespace

int main() {
    // P2 字段绑定 writer
    testWriteWorldMatrixField();
    testWriteMaterialVectorField();
    testWritePackedVector4Field();

    // P2 各 shader 注册表 layout 填充
    testDefaultColorLayout();
    testDefaultImageLayout();
    testImageNormalLayout();
    testFontLayout();
    testBounceLayout();
    testButtonLayout();
    testShaderAliasesShareLayout();

    // P2 安全回退 / 多实例
    testMissingParamsFallbackSafely();
    testMultipleInstancesDoNotOverlap();

    // P3 布局校验
    testValidateLayoutMatches();
    testValidateOffsetMismatch();
    testValidateTypeMismatch();
    testValidateMissingField();
    testValidateStrideMismatch();
    testValidateUnavailableReflection();

    testElementSizesMatchStructs();

    if (g_failures != 0) {
        std::cerr << g_failures << " SSBO layout test(s) failed\n";
        return 1;
    }
    std::cout << "All SSBO layout tests passed\n";
    return 0;
}
