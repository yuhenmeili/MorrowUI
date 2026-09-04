//
// Created by 0060328 on 25-10-23.
//

#include "SSBOFieldBinding.h"

#include <cstring>

#include "Material.h"
#include "SSBOLayoutComponent.h"
#include "ShaderStorageBuffer.h"
#include "base/Transform.h"

namespace morrow {
namespace {
float evalComponentFromMaterial(const SSBOComponentSource& component, const Material& material) {
    switch (component.source) {
        case SSBOValueSource::MaterialFloat:
            return material.getFloatOr(component.materialProperty, 0.0f);
        case SSBOValueSource::MaterialVectorComponent: {
            float value = 0.0f;
            material.tryGetVectorComponent(component.materialProperty, component.sourceComponent, value);
            return value;
        }
        case SSBOValueSource::ConstantFloat:
        default:
            return component.constant;
    }
}

float evalComponent(const SSBOComponentSource& component, const RenderBatch& batch, size_t index) {
    return evalComponentFromMaterial(component, *batch.materials[index]);
}
}  // namespace

size_t shaderDataTypeSize(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Int:
        case ShaderDataType::Float:
            return 4;
        case ShaderDataType::Vector2:
            return 8;
        case ShaderDataType::Vector3:
            return 12;
        case ShaderDataType::Vector4:
            return 16;
        case ShaderDataType::Matrix3:
            return 36;
        case ShaderDataType::Matrix4:
            return 64;
    }
    return 0;
}

bool writeSSBOField(const SSBOFieldBinding& field, void* destination, const RenderBatch& batch, size_t index) {
    auto* dest = static_cast<uint8_t*>(destination) + field.offset;
    switch (field.kind) {
        case SSBOFieldKind::WorldMatrix: {
            const Matrix4& world = batch.transforms[index]->getWorldMatrix();
            std::memcpy(dest, world.elements, sizeof(Matrix4));
            return true;
        }
        case SSBOFieldKind::MaterialVector: {
            const Vector4 value = batch.materials[index]->getVector4Or(field.materialProperty, Vector4::ZERO);
            std::memcpy(dest, value.elements, shaderDataTypeSize(field.type));
            return true;
        }
        case SSBOFieldKind::PackedVector4: {
            float components[4];
            for (int i = 0; i < 4; ++i) {
                components[i] = evalComponent(field.components[i], batch, index);
            }
            std::memcpy(dest, components, sizeof(components));
            return true;
        }
        case SSBOFieldKind::Custom:
        default:
            return false;
    }
}

bool fillSSBOInstance(const SSBOLayout& layout, void* destination, const RenderBatch& batch, size_t index) {
    bool ok = true;
    for (const auto& field : layout.fields) {
        ok = writeSSBOField(field, destination, batch, index) && ok;
    }
    if (layout.filler) {
        layout.filler(destination, batch, static_cast<int>(index));
    }
    return ok;
}

void packSSBOLayoutUniforms(Material& material) {
    const auto layoutComponent = material.getSSBOLayout();
    if (!layoutComponent) {
        return;
    }
    const auto& layout = layoutComponent->getLayout();
    for (const auto& field : layout.fields) {
        switch (field.kind) {
            case SSBOFieldKind::WorldMatrix:
                continue;  // u_model 由 BatchManager 按实例世界矩阵设置
            case SSBOFieldKind::MaterialVector:
                material.setVector(field.shaderField, material.getVector4Or(field.materialProperty, Vector4::ZERO));
                break;
            case SSBOFieldKind::PackedVector4: {
                Vector4 value;
                value.x = evalComponentFromMaterial(field.components[0], material);
                value.y = evalComponentFromMaterial(field.components[1], material);
                value.z = evalComponentFromMaterial(field.components[2], material);
                value.w = evalComponentFromMaterial(field.components[3], material);
                material.setVector(field.shaderField, value);
                break;
            }
            case SSBOFieldKind::Custom:
            default:
                break;
        }
    }
}
}  // namespace morrow