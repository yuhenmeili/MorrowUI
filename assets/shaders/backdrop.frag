// 模糊面片段元着色器：采样共享模糊链 + 9-tap tent 上采样 + 双层插值
// + acrylic 混色 + 圆角遮罩（KAWASE_BACKDROP_BLUR_PROPOSAL.md §5.1 / S3）。
precision mediump float;

in vec2 v_uv;
in vec4 v_tint;
in vec2 v_objPos;
in vec3 v_sizeRound;
in float v_blend;

#include "common/rounded_clip.glsl"

uniform sampler2D u_texture;     // 当前（或双层中的低）层级纹理
uniform sampler2D u_textureNext; // 双层插值的高一层级（单层时绑定同 u_texture）
uniform vec2 u_texel;            // 1 / 当前层级分辨率
uniform vec2 u_texelNext;        // 1 / 高一层级分辨率

layout(location = 0) out vec4 fragColor;

// 9-tap tent 上采样（S3）：低分辨率层级直接双线性放大在近景文字边缘会有
// 2px 阶梯；十字 + 对角 9 tap 加权使上采样平滑。
vec3 sampleTent(sampler2D tex, vec2 uv, vec2 texel) {
    vec3 c = texture(tex, uv).rgb * 4.0;
    c += texture(tex, uv + vec2(1.0, 0.0) * texel).rgb * 2.0;
    c += texture(tex, uv + vec2(-1.0, 0.0) * texel).rgb * 2.0;
    c += texture(tex, uv + vec2(0.0, 1.0) * texel).rgb * 2.0;
    c += texture(tex, uv + vec2(0.0, -1.0) * texel).rgb * 2.0;
    c += texture(tex, uv + vec2(1.0, 1.0) * texel).rgb;
    c += texture(tex, uv + vec2(-1.0, 1.0) * texel).rgb;
    c += texture(tex, uv + vec2(1.0, -1.0) * texel).rgb;
    c += texture(tex, uv + vec2(-1.0, -1.0) * texel).rgb;
    return c / 16.0;
}

void main() {
    applyRoundedClip(v_sizeRound.xy, v_sizeRound.z, vec3(v_objPos, 0.0));

    // 半径动画（S3 双层混合插值）：levelF 的小数部分在相邻两个层级间插值，
    // 层级固定、链不变，只有采样混合因子动画
    vec3 blurred = sampleTent(u_texture, v_uv, u_texel);
    if (v_blend > 0.0001) {
        blurred = mix(blurred, sampleTent(u_textureNext, v_uv, u_texelNext), v_blend);
    }

    // 模糊面片不透明地替换其下的清晰背景（回屏合成恢复的是清晰内容，
    // 若保留 alpha 透出会与模糊版叠加成双重图像）。tint.a 只作为混色强度：
    // 结果 = mix(模糊背景, tint.rgb, tint.a)，即 acrylic 语义。
    fragColor = vec4(mix(blurred, v_tint.rgb, v_tint.a), 1.0);
}
