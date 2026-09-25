// tint 降级面片段元着色器：圆角纯色输出（iOS "降低透明度" 同款形态，
// KAWASE_BACKDROP_BLUR_PROPOSAL.md §5.7）。
precision mediump float;

in vec3 v_position;

#include "common/rounded_clip.glsl"

uniform vec3 u_displaySize; // xy = 面板尺寸
uniform float u_rounding;
uniform float u_alpha;
uniform vec4 u_tintColor;

layout(location = 0) out vec4 fragColor;

void main() {
    applyRoundedClip(u_displaySize.xy, u_rounding, v_position);
    fragColor = vec4(u_tintColor.rgb, u_tintColor.a * u_alpha);
}
