// 统一实例数据 accessor：SSBO/uniform 双路径对外同名函数。
// 依赖宿主阶段先定义 instanceBatchID()（vert 从 a_batch，frag 从 v_batchID）。
#include "common/instance.glsl"

#ifdef ENABLE_SSBO
vec4 instanceColor() {
    return instances[instanceBatchID()].color0;
}
vec4 instanceColor1() {
    return instances[instanceBatchID()].color1;
}
vec2 instanceDisplaySize() {
    return instances[instanceBatchID()].geomAttr.xy;
}
float instanceRounding() {
    return instances[instanceBatchID()].geomAttr.z;
}
float instanceAlpha() {
    return instances[instanceBatchID()].geomAttr.w;
}
vec4 instanceState() {
    return instances[instanceBatchID()].stateAttr;
}
float instanceUseTexture() {
    return instances[instanceBatchID()].stateAttr.x;
}
vec4 instanceExtra() {
    return instances[instanceBatchID()].extraAttr;
}
#else
uniform vec4 u_color0;
uniform vec4 u_color1;
uniform vec4 u_geomAttr;    // xy = displaySize, z = rounding, w = alpha
uniform vec4 u_stateAttr;
uniform vec4 u_extraAttr;
vec4 instanceColor() {
    return u_color0;
}
vec4 instanceColor1() {
    return u_color1;
}
vec2 instanceDisplaySize() {
    return u_geomAttr.xy;
}
float instanceRounding() {
    return u_geomAttr.z;
}
float instanceAlpha() {
    return u_geomAttr.w;
}
vec4 instanceState() {
    return u_stateAttr;
}
float instanceUseTexture() {
    return u_stateAttr.x;
}
vec4 instanceExtra() {
    return u_extraAttr;
}
#endif
