// 模糊面片段元着色器：采样共享模糊链 L0' + acrylic 混色 + 圆角遮罩
//（KAWASE_BACKDROP_BLUR_PROPOSAL.md §5.1 / §5.6）。
precision mediump float;

in vec2 v_uv;
in vec4 v_tint;
in vec2 v_objPos;
in vec3 v_sizeRound;

#include "common/rounded_clip.glsl"

uniform sampler2D u_texture; // 模糊链 L0'

layout(location = 0) out vec4 fragColor;

void main() {
    applyRoundedClip(v_sizeRound.xy, v_sizeRound.z, vec3(v_objPos, 0.0));

    // 模糊背景按 tint.a 与 tint.rgb 混色（acrylic），tint.a 同时作为面片透明度
    vec3 blurred = texture(u_texture, v_uv).rgb;
    vec3 rgb = mix(blurred, v_tint.rgb, v_tint.a);
    fragColor = vec4(rgb, v_tint.a);
}
