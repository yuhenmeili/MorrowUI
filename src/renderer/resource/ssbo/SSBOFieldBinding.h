//
// Created by 0060328 on 25-10-23.
//
// SSBO 字段绑定系统（P2）：
//   用声明式字段描述替代手工 filler，将「CPU 结构字段 ← 数据来源」显式化，
//   并支持 vec4 分量打包；shaderField 同时用于 P3 shader reflection 校验。
//

#ifndef SSBOFIELDBINDING_H
#define SSBOFIELDBINDING_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "core/BatchDataDefine.h"

namespace morrow {
// Shader 属性/字段数据类型（与 GLSL 类型对应）
enum class ShaderDataType { Int, Float, Vector2, Vector3, Vector4, Matrix3, Matrix4 };

// 字段数据来源
enum class SSBOValueSource {
    TransformWorldMatrix,     // 世界矩阵（mat4）
    MaterialFloat,            // Material::getFloat
    MaterialVector2,          // Material vector（2 分量）
    MaterialVector3,          // Material vector（3 分量）
    MaterialVector4,          // Material vector（4 分量）
    MaterialVectorComponent,  // Material vector 的单个分量
    ConstantFloat,            // 常量 float
    Custom                    // 由 SSBOLayout::filler 处理
};

// 字段写入方式
enum class SSBOFieldKind {
    WorldMatrix,     // 直接写入 Transform 世界矩阵（mat4）
    MaterialVector,  // 整个 vector 写入（Vector2/3/4）
    PackedVector4,   // 4 个分量打包为 vec4
    Custom           // 由 SSBOLayout::filler 处理
};

// vec4 单个分量的来源描述（PackedVector4 用）
struct SSBOComponentSource {
    SSBOValueSource source = SSBOValueSource::ConstantFloat;
    std::string materialProperty;  // MaterialFloat / MaterialVectorComponent 用
    int sourceComponent = -1;      // MaterialVectorComponent 用
    float constant = 0.0f;
};

// 字段绑定：描述 CPU 结构字段 ← 数据来源；shaderField 对应 GLSL SSBO 字段名
struct SSBOFieldBinding {
    std::string shaderField;  // 对应 GLSL SSBO 字段名（P3 reflection 校验用）
    size_t offset = 0;        // CPU 结构内字节偏移
    ShaderDataType type = ShaderDataType::Vector4;
    SSBOFieldKind kind = SSBOFieldKind::Custom;
    SSBOValueSource source = SSBOValueSource::Custom;  // MaterialVector 用
    std::string materialProperty;                      // MaterialVector / 直接来源用
    int sourceComponent = -1;                          // MaterialVectorComponent 用
    std::array<SSBOComponentSource, 4> components{};   // PackedVector4 用
};

// ── 构造辅助 ──
inline SSBOFieldBinding makeWorldMatrixField(std::string shaderField, size_t offset) {
    SSBOFieldBinding field;
    field.shaderField = std::move(shaderField);
    field.offset = offset;
    field.type = ShaderDataType::Matrix4;
    field.kind = SSBOFieldKind::WorldMatrix;
    field.source = SSBOValueSource::TransformWorldMatrix;
    return field;
}

inline SSBOFieldBinding makeMaterialVectorField(std::string shaderField, size_t offset, SSBOValueSource source, std::string materialProperty) {
    SSBOFieldBinding field;
    field.shaderField = std::move(shaderField);
    field.offset = offset;
    field.kind = SSBOFieldKind::MaterialVector;
    field.source = source;
    field.materialProperty = std::move(materialProperty);
    switch (source) {
        case SSBOValueSource::MaterialVector2:
            field.type = ShaderDataType::Vector2;
            break;
        case SSBOValueSource::MaterialVector3:
            field.type = ShaderDataType::Vector3;
            break;
        case SSBOValueSource::MaterialVector4:
            field.type = ShaderDataType::Vector4;
            break;
        default:
            field.type = ShaderDataType::Vector4;
            break;
    }
    return field;
}

inline SSBOFieldBinding makePackedVector4Field(std::string shaderField, size_t offset, std::array<SSBOComponentSource, 4> components) {
    SSBOFieldBinding field;
    field.shaderField = std::move(shaderField);
    field.offset = offset;
    field.type = ShaderDataType::Vector4;
    field.kind = SSBOFieldKind::PackedVector4;
    field.source = SSBOValueSource::Custom;
    field.components = std::move(components);
    return field;
}

// ── 分量来源辅助 ──
inline SSBOComponentSource materialFloat(std::string materialProperty) {
    SSBOComponentSource source;
    source.source = SSBOValueSource::MaterialFloat;
    source.materialProperty = std::move(materialProperty);
    return source;
}

inline SSBOComponentSource materialVectorComponent(std::string materialProperty, int component) {
    SSBOComponentSource source;
    source.source = SSBOValueSource::MaterialVectorComponent;
    source.materialProperty = std::move(materialProperty);
    source.sourceComponent = component;
    return source;
}

inline SSBOComponentSource constantFloat(float value) {
    SSBOComponentSource source;
    source.source = SSBOValueSource::ConstantFloat;
    source.constant = value;
    return source;
}

// 通用字段写入：将绑定字段写入 destination 的 field.offset 处（纯 CPU，可测试）。
// 使用 Material 类型安全接口，缺失/类型不匹配时按默认值回退，不抛异常。
bool writeSSBOField(const SSBOFieldBinding& field, void* destination, const RenderBatch& batch, size_t index);

// 填充单个实例：先写入全部 fields，再调用 layout.filler（custom packer）。
// 需要 SSBOLayout 完整定义，实现在 SSBOFieldBinding.cpp。
struct SSBOLayout;

bool fillSSBOInstance(const SSBOLayout& layout, void* destination, const RenderBatch& batch, size_t index);

class Material;

// 非 SSBO 路径的统一 uniform 打包：按 material 的 SSBO 绑定表把属性值打包为
// u_color0 / u_geomAttr / ... 通用槽位 uniform（与 GLSL instance.frag.glsl 的
// 非 SSBO 分支一一对应）。u_model 由 BatchManager 单独设置。无 layout 时无操作。
void packSSBOLayoutUniforms(Material& material);

// 布局校验辅助（P3 复用）
size_t shaderDataTypeSize(ShaderDataType type);
}  // namespace morrow

#endif  // SSBOFIELDBINDING_H