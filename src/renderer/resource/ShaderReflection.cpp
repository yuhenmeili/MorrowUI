//
// Created by 0060328 on 25-10-23.
//

#include "ShaderReflection.h"

namespace morrow {

std::string validateSSBOLayout(const SSBOLayout& cpuLayout, const SSBOReflectedLayout& glslLayout) {
    if (!glslLayout.valid) {
        // 反射不可用（block 不存在或驱动不支持），无法校验，由调用方决定是否告警
        return {};
    }

    // 1. 元素 stride 校验
    if (cpuLayout.elementSize > 0 && glslLayout.topLevelArrayStride > 0 && cpuLayout.elementSize != static_cast<size_t>(glslLayout.topLevelArrayStride)) {
        return "element size mismatch: cpu=" + std::to_string(cpuLayout.elementSize) + " glsl=" + std::to_string(glslLayout.topLevelArrayStride);
    }

    // 2. 字段偏移 / 类型 / 越界校验
    for (const auto& field : cpuLayout.fields) {
        if (field.kind == SSBOFieldKind::Custom) {
            continue;  // custom packer 字段不参与反射校验
        }

        const SSBOReflectedField* reflected = nullptr;
        for (const auto& candidate : glslLayout.fields) {
            if (candidate.name == field.shaderField) {
                reflected = &candidate;
                break;
            }
        }
        if (reflected == nullptr) {
            return "GLSL missing field '" + field.shaderField + "'";
        }

        if (reflected->offset >= 0 && static_cast<size_t>(reflected->offset) != field.offset) {
            return "field '" + field.shaderField + "' offset mismatch: cpu=" + std::to_string(field.offset) + " glsl=" + std::to_string(reflected->offset);
        }

        if (reflected->type != field.type) {
            return "field '" + field.shaderField + "' type mismatch";
        }

        if (cpuLayout.elementSize > 0) {
            const size_t fieldSize = shaderDataTypeSize(field.type);
            if (field.offset + fieldSize > cpuLayout.elementSize) {
                return "field '" + field.shaderField + "' overflows element size";
            }
        }
    }

    return {};
}

}  // namespace morrow
