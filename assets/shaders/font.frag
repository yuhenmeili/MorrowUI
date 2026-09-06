layout (location = 0) in vec2 v_texCoord;
layout (location = 1) flat in int v_batchID;

#include "common/instance.frag.glsl"

uniform sampler2D u_texture;

layout (location = 0) out vec4 fragColor;

// SDF 重建参数（必须与 DynamicFont.h 中 SDF_ONEDGE / SDF_PIXEL_DIST_SCALE 一致）
const float SDF_EDGE = 0.50196f;        // 128 / 255
const float SDF_PX_PER_UNIT = 15.9375f; // 255 / 16

void main() {
    vec4 fontColor = instanceColor();
    // SDF -> coverage：距边缘 ±0.5px 内线性过渡，等效抗锯齿。
    // sdfScale（目标字号 / 参考字号 44px）经实例槽位 stateAttr.x 逐实例传入，
    // 混合字号的同一批次内每个实例使用各自的缩放系数。
    float d = (texture(u_texture, v_texCoord.st).r - SDF_EDGE) * SDF_PX_PER_UNIT;
    float coverage = smoothstep(-0.5, 0.5, d * instanceState().x);
    fragColor = vec4(fontColor.rgb, coverage * fontColor.a * instanceAlpha());
}
