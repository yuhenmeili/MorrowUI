#pragma once

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

#include "ShaderStorageBuffer.h"
#include "SSBOFieldBinding.h"

namespace morrow {

template <std::size_t AttributeCount>
struct DefaultBatchData {
    Matrix4 model;
    Vector4 attrs[AttributeCount];
};

using DefaultBatchData1Attr = DefaultBatchData<1>;
using DefaultBatchData2Attr = DefaultBatchData<2>;
using DefaultBatchData3Attr = DefaultBatchData<3>;

template <typename T>
constexpr std::size_t defaultBatchAttributeOffset(std::size_t index) {
    return offsetof(T, attrs) + sizeof(Vector4) * index;
}

// ── 统一 UI 实例布局（SHADER_SOURCE_ORGANIZATION_PROPOSAL.md §4）──
// 全部 UI 组件共用一个结构；CPU 侧每帧全量填充，未使用的槽位由绑定声明写默认值：
// color0/color1 = (1,1,1,1)，geomAttr = (0,0,0,1)，stateAttr/extraAttr = (0,0,0,0)。
// GLSL 侧唯一定义在 assets/shaders/common/instance.glsl，字段一一对应。
struct UIInstanceData {
    Matrix4 model;
    Vector4 color0;     //主色：bgColor / fontColor / defaultColor / trackColor ...
    Vector4 color1;     //副色或辅助向量：fillColor / meshCenter.xyz + alpha
    Vector4 geomAttr;   //xy = displaySize, z = rounding, w = alpha
    Vector4 stateAttr;  //状态/开关：useTexture / progress / direction ...
    Vector4 extraAttr;  //自定义参数：动画参数等
};

static_assert(sizeof(Matrix4) == 64, "Matrix4 must be 64 bytes to match GLSL mat4");
static_assert(sizeof(Vector4) == 16, "Vector4 must be 16 bytes to match GLSL vec4");

static_assert(sizeof(UIInstanceData) == 144, "UIInstanceData must be 144 bytes (mat4 + 5 x vec4)");
static_assert(offsetof(UIInstanceData, model) == 0, "model must be at offset 0");
static_assert(offsetof(UIInstanceData, color0) == 64, "color0 must be at offset 64");
static_assert(offsetof(UIInstanceData, color1) == 80, "color1 must be at offset 80");
static_assert(offsetof(UIInstanceData, geomAttr) == 96, "geomAttr must be at offset 96");
static_assert(offsetof(UIInstanceData, stateAttr) == 112, "stateAttr must be at offset 112");
static_assert(offsetof(UIInstanceData, extraAttr) == 128, "extraAttr must be at offset 128");
static_assert(std::is_standard_layout_v<UIInstanceData>, "UIInstanceData must be standard layout");

static_assert(offsetof(DefaultBatchData1Attr, model) == 0, "model must be at offset 0");
static_assert(defaultBatchAttributeOffset<DefaultBatchData1Attr>(0) == 64, "attr must be at offset 64");
static_assert(sizeof(DefaultBatchData1Attr) == 80, "DefaultBatchData1Attr must be 80 bytes");

static_assert(offsetof(DefaultBatchData2Attr, model) == 0, "model must be at offset 0");
static_assert(defaultBatchAttributeOffset<DefaultBatchData2Attr>(0) == 64, "attr1 must be at offset 64");
static_assert(defaultBatchAttributeOffset<DefaultBatchData2Attr>(1) == 80, "attr2 must be at offset 80");
static_assert(sizeof(DefaultBatchData2Attr) == 96, "DefaultBatchData2Attr must be 96 bytes");

static_assert(offsetof(DefaultBatchData3Attr, model) == 0, "model must be at offset 0");
static_assert(defaultBatchAttributeOffset<DefaultBatchData3Attr>(0) == 64, "attr1 must be at offset 64");
static_assert(defaultBatchAttributeOffset<DefaultBatchData3Attr>(1) == 80, "attr2 must be at offset 80");
static_assert(defaultBatchAttributeOffset<DefaultBatchData3Attr>(2) == 96, "attr3 must be at offset 96");
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

// ── 统一布局槽位构造辅助（未使用槽位写确定默认值，保证全量填充）──
// 颜色槽位默认：(1,1,1,1)（乘法中性色）
inline std::array<SSBOComponentSource, 4> defaultColorSlot() {
    return {constantFloat(1.0f), constantFloat(1.0f), constantFloat(1.0f), constantFloat(1.0f)};
}

// 几何槽位默认：(0, 0, 0, 1)（零尺寸、无圆角、不透明）
inline std::array<SSBOComponentSource, 4> defaultGeomSlot() {
    return {constantFloat(0.0f), constantFloat(0.0f), constantFloat(0.0f), constantFloat(1.0f)};
}

// 全零槽位：stateAttr / extraAttr 默认
inline std::array<SSBOComponentSource, 4> zeroSlot() {
    return {constantFloat(0.0f), constantFloat(0.0f), constantFloat(0.0f), constantFloat(0.0f)};
}

// 几何槽位（仅 alpha）：(0, 0, 0, alpha)
inline std::array<SSBOComponentSource, 4> alphaGeomSlot() {
    return {constantFloat(0.0f), constantFloat(0.0f), constantFloat(0.0f), materialFloat("alpha")};
}

// 几何槽位（标准）：(displaySize.x, displaySize.y, rounding, alpha)
inline std::array<SSBOComponentSource, 4> displayGeomSlot() {
    return {materialVectorComponent("displaySize", 0), materialVectorComponent("displaySize", 1), materialFloat("rounding"), materialFloat("alpha")};
}

// UIInstanceData 字段构造快捷方式
inline SSBOFieldBinding uiSlotField(std::string shaderField, size_t offset, std::array<SSBOComponentSource, 4> components) {
    return makePackedVector4Field(std::move(shaderField), offset, std::move(components));
}

}  // namespace morrow
