//
// Created by 0060328 on 25-10-23.
//
// SSBO shader reflection 校验（P3）：
//   SSBOReflectedLayout 描述 GLSL 侧 SSBO block 的字段布局（由 GL 查询填充）；
//   validateSSBOLayout 将 CPU 注册的 layout 与 GLSL 反射结果对比，
//   防止 CPU/GLSL 字段偏移、类型或元素 stride 静默错位。
//   本文件只含纯 CPU 逻辑，不依赖 GL 头；GL 查询在 GLRenderDevice 实现。
//

#ifndef SHADERREFLECTION_H
#define SHADERREFLECTION_H

#include <cstdint>
#include <string>
#include <vector>

#include "ssbo/ShaderStorageBuffer.h"

namespace morrow {

// OpenGL program 反射出的 SSBO block 布局
struct SSBOReflectedField {
    std::string name;   // GLSL 字段名（如 model / bgColor）
    ShaderDataType type = ShaderDataType::Float;
    int32_t offset = -1;         // 相对 block 起始的字节偏移
    int32_t arrayStride = -1;    // 数组字段的 stride
    int32_t matrixStride = -1;   // 矩阵字段的 stride
};

struct SSBOReflectedLayout {
    bool valid = false;                 // 反射是否成功（block 存在）
    int32_t topLevelArrayStride = -1;   // instances[] 顶层数组元素 stride
    std::vector<SSBOReflectedField> fields;
};

// 纯 CPU 校验：CPU layout 与 GLSL 反射结果对比。
// 返回错误描述（空字符串 = 一致 / 无法校验）。glslLayout.valid == false 时返回空。
std::string validateSSBOLayout(const SSBOLayout& cpuLayout, const SSBOReflectedLayout& glslLayout);

} // namespace morrow

#endif //SHADERREFLECTION_H
