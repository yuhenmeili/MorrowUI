layout (location = 0) in vec2 v_texCoord;
layout (location = 1) flat in int v_batchID;

#include "common/instance.frag.glsl"

uniform sampler2D u_texture;

layout (location = 0) out vec4 fragColor;

// SDF 重建参数（必须与 DynamicFont.h 中 SDF_ONEDGE / SDF_PIXEL_DIST_SCALE 一致）
const float SDF_EDGE = 0.70588f;       // 180 / 255
const float SDF_PX_PER_UNIT = 7.0833f; // 255 / 36

void main() {
    vec4 fontColor = instanceColor();
    // SDF -> coverage：距边缘 ±0.5px 内线性过渡，等效抗锯齿
    float d = (texture(u_texture, v_texCoord.st).r - SDF_EDGE) * SDF_PX_PER_UNIT;
    float coverage = smoothstep(-0.5, 0.5, d);
    fragColor = vec4(fontColor.rgb, coverage * fontColor.a * instanceAlpha());
}
