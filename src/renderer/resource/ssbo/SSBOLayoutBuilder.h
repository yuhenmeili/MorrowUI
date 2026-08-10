#pragma once

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

#include "ShaderStorageBuffer.h"
#include "SSBOFieldBinding.h"

namespace morrow {

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

template <typename T>
SSBOLayout makeLayout(std::string name) {
    static_assert(std::is_standard_layout_v<T>, "SSBO instance data must be standard layout");

    SSBOLayout layout;
    layout.name = std::move(name);
    layout.elementSize = sizeof(T);
    return layout;
}

}  // namespace morrow
