// 模糊面片段元着色器：采样共享模糊链 + acrylic 混色 + 圆角遮罩
//（KAWASE_BACKDROP_BLUR_PROPOSAL.md §5.1 / §5.6）。
precision mediump float;

in vec2 v_uv;
in vec4 v_tint;
in vec2 v_objPos;
in vec3 v_sizeRound;

#include "common/rounded_clip.glsl"

uniform sampler2D u_texture; // 当前层级采样纹理（L0'/L1'/L2' 之一）

layout(location = 0) out vec4 fragColor;

void main() {
    applyRoundedClip(v_sizeRound.xy, v_sizeRound.z, vec3(v_objPos, 0.0));

    // 模糊面片不透明地替换其下的清晰背景（回屏合成恢复的是清晰内容，
    // 若保留 alpha 透出会与模糊版叠加成双重图像）。tint.a 只作为混色强度：
    // 结果 = mix(模糊背景, tint.rgb, tint.a)，即 acrylic 语义。
    vec3 blurred = texture(u_texture, v_uv).rgb;
    fragColor = vec4(mix(blurred, v_tint.rgb, v_tint.a), 1.0);
}
