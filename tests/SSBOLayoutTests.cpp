//
// 统一 UI 实例布局（SHADER_SOURCE_ORGANIZATION_PROPOSAL.md）纯 CPU 单元测试：
//   P2/P3 原有覆盖（字段绑定填充 / validateSSBOLayout）按统一 UIInstanceData 更新；
//   新增 include 解析（resolveIncludes）与非 SSBO uniform 打包（packSSBOLayoutUniforms）。
//
// 只验证 CPU 侧（不创建 GPU 资源，不经过 updateSSBOForShader / GPU 编译）。
//

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/BatchDataDefine.h"
#include "renderer/resource/MaterialUtil.h"
#include "renderer/resource/ssbo/SSBOFieldBinding.h"
#include "renderer/resource/ssbo/SSBOLayoutBuilder.h"
#include "renderer/resource/ssbo/layouts/BounceSSBOLayout.h"
#include "renderer/resource/ssbo/layouts/ButtonSSBOLayout.h"
#include "renderer/resource/ssbo/layouts/DefaultColorSSBOLayout.h"
#include "renderer/resource/ssbo/layouts/DefaultImageSSBOLayout.h"
#include "renderer/resource/ssbo/layouts/FontSSBOLayout.h"
#include "renderer/resource/ssbo/layouts/ImageSSBOLayout.h"
#include "renderer/resource/ssbo/layouts/ProgressBarSSBOLayout.h"
#include "renderer/resource/ssbo/layouts/TextureButtonSSBOLayout.h"
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

const SSBOLayout* getLayoutForTest(const std::string& name) {
    static const DefaultColorSSBOLayout defaultColor;
    static const DefaultImageSSBOLayout defaultImage;
    static const ImageSSBOLayout image;
    static const FontSSBOLayout font;
    static const BounceSSBOLayout bounce;
    static const ButtonSSBOLayout button;
    static const TextureButtonSSBOLayout textureButton;
    static const ProgressBarSSBOLayout progressBar;

    if (name == "default_color") return &defaultColor.getLayout();
    if (name == "default_image") return &defaultImage.getLayout();
    if (name == "image_normal" || name == "image_text_debug" || name == "image_oes") return &image.getLayout();
    if (name == "font") return &font.getLayout();
    if (name == "bounce") return &bounce.getLayout();
    if (name == "button") return &button.getLayout();
    if (name == "texture_button") return &textureButton.getLayout();
    if (name == "progress_bar") return &progressBar.getLayout();
    return nullptr;
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
// 字段绑定 writer
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
        "color0", 0, SSBOValueSource::MaterialVector4, "color");
    uint8_t buffer[sizeof(Vector4)]{};
    expect(writeSSBOField(field, buffer, batch, 0), "material vector field should write");
    const auto* value = reinterpret_cast<const Vector4*>(buffer);
    expectVec4(*value, 1.0f, 2.0f, 3.0f, 4.0f, "material vector4 should be written");
}

void testWritePackedVector4Field() {
    auto batch = makeBatch(1);
    batch.materials[0]->setVector("displaySize", Vector3(10.0f, 20.0f, 30.0f));
    batch.materials[0]->setFloat("rounding", 0.3f);
    const auto field = makePackedVector4Field("geomAttr", 0, {
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
// 统一布局：各 shader 绑定填充（含未使用槽位默认值）
// ════════════════════════════════════════════════════════════════════

void testUnifiedElementSize() {
    for (const char* name : {"default_color", "default_image", "image_normal", "font", "bounce", "button", "texture_button", "progress_bar"}) {
        const auto* layout = getLayoutForTest(name);
        expect(layout != nullptr, std::string(name) + " layout should be registered");
        if (!layout) continue;
        expect(layout->elementSize == sizeof(UIInstanceData),
               std::string(name) + " should use the unified 144-byte element size");
        expect(layout->fields.size() == 6,
               std::string(name) + " should declare all 6 unified slots");
    }
}

void testDefaultColorLayout() {
    auto batch = makeBatch(1);
    batch.materials[0]->setVector("color", Vector4(1.0f, 2.0f, 3.0f, 4.0f));
    batch.materials[0]->setFloat("alpha", 0.5f);

    const auto* layout = getLayoutForTest("default_color");
    expect(layout != nullptr, "default_color layout should be registered");
    if (!layout) return;
    std::vector<uint8_t> buffer(layout->elementSize, 0xAA);
    fillSSBOInstance(*layout, buffer.data(), batch, 0);

    const auto* data = reinterpret_cast<const UIInstanceData*>(buffer.data());
    expect(sameMatrix(data->model, batch.transforms[0]->getWorldMatrix()),
           "default_color: model matrix should match");
    expectVec4(data->color0, 1.0f, 2.0f, 3.0f, 4.0f, "default_color: color0 should be color");
    expectVec4(data->color1, 1.0f, 1.0f, 1.0f, 1.0f, "default_color: unused color1 should be default (1,1,1,1)");
    expectVec4(data->geomAttr, 0.0f, 0.0f, 0.0f, 0.5f, "default_color: geomAttr.w should be alpha");
    expectVec4(data->stateAttr, 0.0f, 0.0f, 0.0f, 0.0f, "default_color: unused stateAttr should be zero");
    expectVec4(data->extraAttr, 0.0f, 0.0f, 0.0f, 0.0f, "default_color: unused extraAttr should be zero");
}

void testImageNormalLayout() {
    auto batch = makeBatch(1);
    batch.materials[0]->setVector("displaySize", Vector3(10.0f, 20.0f, 30.0f));
    batch.materials[0]->setFloat("rounding", 0.3f);
    batch.materials[0]->setFloat("alpha", 0.6f);

    const auto* layout = getLayoutForTest("image_normal");
    expect(layout != nullptr, "image_normal layout should be registered");
    if (!layout) return;
    std::vector<uint8_t> buffer(layout->elementSize, 0xAA);
    fillSSBOInstance(*layout, buffer.data(), batch, 0);

    const auto* data = reinterpret_cast<const UIInstanceData*>(buffer.data());
    expect(sameMatrix(data->model, batch.transforms[0]->getWorldMatrix()),
           "image_normal: model matrix should match");
    expectVec4(data->color0, 1.0f, 1.0f, 1.0f, 1.0f, "image_normal: unused color0 should be default");
    expectVec4(data->geomAttr, 10.0f, 20.0f, 0.3f, 0.6f,
               "image_normal: geomAttr should be (displaySize.xy, rounding, alpha)");
    expectVec4(data->extraAttr, 0.0f, 0.0f, 0.0f, 0.0f, "image_normal: unused extraAttr should be zero");
}

void testBounceLayout() {
    auto batch = makeBatch(1);
    batch.materials[0]->setVector("meshCenter", Vector3(5.0f, 6.0f, 7.0f));
    batch.materials[0]->setFloat("alpha", 0.9f);
    batch.materials[0]->setFloat("timeDelta", 1.0f);
    batch.materials[0]->setFloat("duration", 2.0f);
    batch.materials[0]->setFloat("bounceTimes", 3.0f);
    batch.materials[0]->setFloat("scaleRange", 4.0f);

    const auto* layout = getLayoutForTest("bounce");
    expect(layout != nullptr, "bounce layout should be registered");
    if (!layout) return;
    std::vector<uint8_t> buffer(layout->elementSize, 0xAA);
    fillSSBOInstance(*layout, buffer.data(), batch, 0);

    const auto* data = reinterpret_cast<const UIInstanceData*>(buffer.data());
    expectVec4(data->color1, 5.0f, 6.0f, 7.0f, 0.9f, "bounce: color1 should be (meshCenter, alpha)");
    expectVec4(data->extraAttr, 1.0f, 2.0f, 3.0f, 4.0f, "bounce: extraAttr should be animation params");
    expectVec4(data->geomAttr, 0.0f, 0.0f, 0.0f, 1.0f, "bounce: unused geomAttr should be (0,0,0,1)");
}

void testButtonLayout() {
    auto batch = makeBatch(1);
    batch.materials[0]->setVector("color", Vector4(0.0f, 1.0f, 0.0f, 1.0f));
    batch.materials[0]->setVector("displaySize", Vector3(100.0f, 200.0f, 0.0f));
    batch.materials[0]->setFloat("rounding", 0.15f);
    batch.materials[0]->setFloat("alpha", 0.7f);
    batch.materials[0]->setFloat("useTexture", 1.0f);

    const auto* layout = getLayoutForTest("button");
    expect(layout != nullptr, "button layout should be registered");
    if (!layout) return;
    std::vector<uint8_t> buffer(layout->elementSize, 0xAA);
    fillSSBOInstance(*layout, buffer.data(), batch, 0);

    const auto* data = reinterpret_cast<const UIInstanceData*>(buffer.data());
    expectVec4(data->color0, 0.0f, 1.0f, 0.0f, 1.0f, "button: color0 should be color");
    expectVec4(data->geomAttr, 100.0f, 200.0f, 0.15f, 0.7f,
               "button: geomAttr should be (displaySize, rounding, alpha)");
    expectVec4(data->stateAttr, 1.0f, 0.0f, 0.0f, 0.0f, "button: stateAttr.x should be useTexture");
}

void testProgressBarLayout() {
    auto batch = makeBatch(1);
    batch.materials[0]->setVector("trackColor", Vector4(0.2f, 0.2f, 0.2f, 1.0f));
    batch.materials[0]->setVector("fillColor", Vector4(0.1f, 0.9f, 0.3f, 1.0f));
    batch.materials[0]->setFloat("progress", 0.75f);
    batch.materials[0]->setFloat("direction", 2.0f);
    batch.materials[0]->setFloat("useTrackTexture", 0.0f);
    batch.materials[0]->setFloat("useFillTexture", 1.0f);

    const auto* layout = getLayoutForTest("progress_bar");
    expect(layout != nullptr, "progress_bar layout should be registered");
    if (!layout) return;
    std::vector<uint8_t> buffer(layout->elementSize, 0xAA);
    fillSSBOInstance(*layout, buffer.data(), batch, 0);

    const auto* data = reinterpret_cast<const UIInstanceData*>(buffer.data());
    expectVec4(data->color0, 0.2f, 0.2f, 0.2f, 1.0f, "progress_bar: color0 should be trackColor");
    expectVec4(data->color1, 0.1f, 0.9f, 0.3f, 1.0f, "progress_bar: color1 should be fillColor");
    expectVec4(data->stateAttr, 0.75f, 2.0f, 0.0f, 1.0f,
               "progress_bar: stateAttr should be (progress, direction, useTrack, useFill)");
    expectVec4(data->extraAttr, 0.0f, 0.0f, 0.0f, 0.0f, "progress_bar: unused extraAttr should be zero");
}

// 别名 shader 共享同一 layout（image_normal / image_text_debug / image_oes）
void testShaderAliasesShareLayout() {
    const auto* a = getLayoutForTest("image_normal");
    const auto* b = getLayoutForTest("image_text_debug");
    const auto* c = getLayoutForTest("image_oes");
    expect(a != nullptr && b != nullptr && c != nullptr,
           "image shader aliases should be registered");
    expect(a == b && b == c, "image shader aliases should share the same layout instance");
}

// ════════════════════════════════════════════════════════════════════
// 缺失参数安全回退（未使用槽位仍是确定默认值，无残留垃圾）
// ════════════════════════════════════════════════════════════════════

void testMissingParamsFallbackSafely() {
    auto batch = makeBatch(1);  // 未设置任何 Material 参数

    const auto* layout = getLayoutForTest("image_normal");
    std::vector<uint8_t> buffer(layout->elementSize, 0xAA);  // 模拟回收池脏数据
    fillSSBOInstance(*layout, buffer.data(), batch, 0);      // 不应抛异常

    const auto* data = reinterpret_cast<const UIInstanceData*>(buffer.data());
    expectVec4(data->color0, 1.0f, 1.0f, 1.0f, 1.0f,
               "missing params: color0 should still be the deterministic default");
    expectVec4(data->geomAttr, 0.0f, 0.0f, 0.0f, 0.0f,
               "missing params: geomAttr components should fall back to zero");
    expectVec4(data->stateAttr, 0.0f, 0.0f, 0.0f, 0.0f,
               "missing params: stateAttr should be zero (no stale bytes)");
    expectVec4(data->extraAttr, 0.0f, 0.0f, 0.0f, 0.0f,
               "missing params: extraAttr should be zero (no stale bytes)");
}

// ════════════════════════════════════════════════════════════════════
// 多实例写入互不覆盖
// ════════════════════════════════════════════════════════════════════

void testMultipleInstancesDoNotOverlap() {
    constexpr size_t count = 3;
    auto batch = makeBatch(count);
    for (size_t i = 0; i < count; i++) {
        batch.materials[i]->setFloat("alpha", static_cast<float>(i) + 1.0f);
    }

    const auto* layout = getLayoutForTest("default_color");
    std::vector<uint8_t> buffer(count * layout->elementSize, 0xAA);
    for (size_t i = 0; i < count; i++) {
        fillSSBOInstance(*layout, buffer.data() + i * layout->elementSize, batch, i);
    }

    for (size_t i = 0; i < count; i++) {
        const auto* instance = reinterpret_cast<const UIInstanceData*>(
            buffer.data() + i * layout->elementSize);
        expect(sameMatrix(instance->model, batch.transforms[i]->getWorldMatrix()),
               "multi-instance: each model matrix should match its own transform");
        expectVec4(instance->geomAttr, 0.0f, 0.0f, 0.0f, static_cast<float>(i) + 1.0f,
                   "multi-instance: each alpha should be independent");
    }
}

// ════════════════════════════════════════════════════════════════════
// validateSSBOLayout 纯 CPU 校验（统一结构期望值）
// ════════════════════════════════════════════════════════════════════

SSBOReflectedLayout makeMatchingGlslLayout() {
    SSBOReflectedLayout glsl;
    glsl.valid = true;
    glsl.topLevelArrayStride = sizeof(UIInstanceData);
    glsl.fields = {
        {"model", ShaderDataType::Matrix4, 0},
        {"color0", ShaderDataType::Vector4, 64},
        {"color1", ShaderDataType::Vector4, 80},
        {"geomAttr", ShaderDataType::Vector4, 96},
        {"stateAttr", ShaderDataType::Vector4, 112},
        {"extraAttr", ShaderDataType::Vector4, 128},
    };
    return glsl;
}

void testValidateLayoutMatches() {
    const auto* cpu = getLayoutForTest("default_color");
    expect(validateSSBOLayout(*cpu, makeMatchingGlslLayout()).empty(),
           "matching unified CPU/GLSL layout should pass validation");
}

void testValidateOffsetMismatch() {
    const auto* cpu = getLayoutForTest("default_color");
    auto glsl = makeMatchingGlslLayout();
    glsl.fields[1].offset = 63;
    const std::string error = validateSSBOLayout(*cpu, glsl);
    expect(!error.empty() && error.find("color0") != std::string::npos,
           "offset mismatch should report the field name");
}

void testValidateTypeMismatch() {
    const auto* cpu = getLayoutForTest("default_color");
    auto glsl = makeMatchingGlslLayout();
    glsl.fields[0].type = ShaderDataType::Vector4;  // model 应为 mat4
    expect(!validateSSBOLayout(*cpu, glsl).empty(),
           "type mismatch should fail validation");
}

void testValidateMissingField() {
    const auto* cpu = getLayoutForTest("default_color");
    auto glsl = makeMatchingGlslLayout();
    glsl.fields.pop_back();  // 缺少 extraAttr
    const std::string error = validateSSBOLayout(*cpu, glsl);
    expect(!error.empty() && error.find("extraAttr") != std::string::npos,
           "missing GLSL field should report the field name");
}

void testValidateStrideMismatch() {
    const auto* cpu = getLayoutForTest("default_color");
    auto glsl = makeMatchingGlslLayout();
    glsl.topLevelArrayStride = 96;  // 与 elementSize 144 不一致
    expect(!validateSSBOLayout(*cpu, glsl).empty(),
           "element stride mismatch should fail validation");
}

void testValidateUnavailableReflection() {
    const auto* cpu = getLayoutForTest("default_color");
    SSBOReflectedLayout glsl;  // valid = false
    expect(validateSSBOLayout(*cpu, glsl).empty(),
           "unavailable reflection should not fail validation");
}

// ════════════════════════════════════════════════════════════════════
// include 解析（resolveIncludes）
// ════════════════════════════════════════════════════════════════════

MaterialUtil::ShaderChunkLoader makeMapLoader(const std::map<std::string, std::string>& chunks) {
    return [&chunks](const std::string& fileName, std::string& source) {
        const auto it = chunks.find(fileName);
        if (it == chunks.end()) {
            return false;
        }
        source = it->second;
        return true;
    };
}

void testResolveIncludesPassthrough() {
    std::string resolved, error;
    expect(MaterialUtil::resolveIncludes("void main() {}\n", makeMapLoader({}), resolved, error),
           "source without includes should resolve");
    expect(resolved == "void main() {}\n", "passthrough should keep content");
}

void testResolveIncludesExpansion() {
    const std::map<std::string, std::string> chunks = {
        {"common/global.glsl", "uniform Global;\n"},
    };
    std::string resolved, error;
    expect(MaterialUtil::resolveIncludes("#include \"common/global.glsl\"\nvoid main() {}\n", makeMapLoader(chunks), resolved, error),
           "simple include should resolve");
    expect(resolved == "uniform Global;\nvoid main() {}\n", "include should be expanded in place");
}

void testResolveIncludesNestedAndOnce() {
    const std::map<std::string, std::string> chunks = {
        {"a.glsl", "#include \"b.glsl\"\nint a;\n#include \"b.glsl\"\n"},
        {"b.glsl", "int b;\n"},
    };
    std::string resolved, error;
    expect(MaterialUtil::resolveIncludes("#include \"a.glsl\"\n", makeMapLoader(chunks), resolved, error),
           "nested include should resolve");
    expect(resolved == "int b;\nint a;\n", "include-once should skip the second expansion of b.glsl");
}

void testResolveIncludesCycleFails() {
    const std::map<std::string, std::string> chunks = {
        {"a.glsl", "#include \"b.glsl\"\n"},
        {"b.glsl", "#include \"a.glsl\"\n"},
    };
    std::string resolved, error;
    expect(!MaterialUtil::resolveIncludes("#include \"a.glsl\"\n", makeMapLoader(chunks), resolved, error),
           "cyclic include should fail");
    expect(!error.empty(), "cycle failure should produce an error message");
}

void testResolveIncludesMissingFails() {
    std::string resolved, error;
    expect(!MaterialUtil::resolveIncludes("#include \"common/missing.glsl\"\n", makeMapLoader({}), resolved, error),
           "missing include should fail");
    expect(error.find("missing.glsl") != std::string::npos,
           "missing include error should name the file");
}

// ════════════════════════════════════════════════════════════════════
// 非 SSBO 路径 uniform 打包（packSSBOLayoutUniforms）
// ════════════════════════════════════════════════════════════════════

void testPackUniformsFromImageLayout() {
    auto material = Material::create();
    material->setSSBOLayout(std::make_shared<ImageSSBOLayout>());
    material->setVector("displaySize", Vector3(30.0f, 40.0f, 0.0f));
    material->setFloat("rounding", 0.25f);
    material->setFloat("alpha", 0.5f);

    packSSBOLayoutUniforms(*material);
    expectVec4(material->getVector4Or("geomAttr", Vector4::ZERO), 30.0f, 40.0f, 0.25f, 0.5f,
               "pack: geomAttr uniform should mirror the SSBO slot packing");
    expectVec4(material->getVector4Or("color0", Vector4::ZERO), 1.0f, 1.0f, 1.0f, 1.0f,
               "pack: unused color0 uniform should get the default value");
    expectVec4(material->getVector4Or("stateAttr", Vector4::ZERO), 0.0f, 0.0f, 0.0f, 0.0f,
               "pack: unused stateAttr uniform should be zero");
}

void testPackUniformsWithoutLayoutIsNoop() {
    auto material = Material::create();
    material->setFloat("alpha", 0.5f);
    packSSBOLayoutUniforms(*material);
    expectVec4(material->getVector4Or("geomAttr", Vector4::ZERO), 0.0f, 0.0f, 0.0f, 0.0f,
               "pack: material without layout should not gain packed uniforms");
}

void testPackUniformsFromButtonLayout() {
    auto material = Material::create();
    material->setSSBOLayout(std::make_shared<ButtonSSBOLayout>());
    material->setVector("color", Vector4(0.1f, 0.2f, 0.3f, 1.0f));
    material->setFloat("useTexture", 1.0f);

    packSSBOLayoutUniforms(*material);
    expectVec4(material->getVector4Or("color0", Vector4::ZERO), 0.1f, 0.2f, 0.3f, 1.0f,
               "pack: button color0 uniform should be color");
    expectVec4(material->getVector4Or("stateAttr", Vector4::ZERO), 1.0f, 0.0f, 0.0f, 0.0f,
               "pack: button stateAttr.x should be useTexture");
}

// ════════════════════════════════════════════════════════════════════
// 布局尺寸
// ════════════════════════════════════════════════════════════════════

void testElementSizesMatchStructs() {
    expect(sizeof(UIInstanceData) == 144, "UIInstanceData must be 144 bytes");
    expect(offsetof(UIInstanceData, color0) == 64, "color0 must be at offset 64");
    expect(offsetof(UIInstanceData, extraAttr) == 128, "extraAttr must be at offset 128");
    expect(shaderDataTypeSize(ShaderDataType::Matrix4) == 64, "mat4 must be 64 bytes");
    expect(shaderDataTypeSize(ShaderDataType::Vector4) == 16, "vec4 must be 16 bytes");
}

} // namespace

int main() {
    // 字段绑定 writer
    testWriteWorldMatrixField();
    testWriteMaterialVectorField();
    testWritePackedVector4Field();

    // 统一布局填充
    testUnifiedElementSize();
    testDefaultColorLayout();
    testImageNormalLayout();
    testBounceLayout();
    testButtonLayout();
    testProgressBarLayout();
    testShaderAliasesShareLayout();

    // 安全回退 / 多实例
    testMissingParamsFallbackSafely();
    testMultipleInstancesDoNotOverlap();

    // P3 布局校验
    testValidateLayoutMatches();
    testValidateOffsetMismatch();
    testValidateTypeMismatch();
    testValidateMissingField();
    testValidateStrideMismatch();
    testValidateUnavailableReflection();

    // include 解析
    testResolveIncludesPassthrough();
    testResolveIncludesExpansion();
    testResolveIncludesNestedAndOnce();
    testResolveIncludesCycleFails();
    testResolveIncludesMissingFails();

    // 非 SSBO uniform 打包
    testPackUniformsFromImageLayout();
    testPackUniformsWithoutLayoutIsNoop();
    testPackUniformsFromButtonLayout();

    testElementSizesMatchStructs();

    if (g_failures != 0) {
        std::cerr << g_failures << " SSBO layout test(s) failed\n";
        return 1;
    }
    std::cout << "All SSBO layout tests passed\n";
    return 0;
}
