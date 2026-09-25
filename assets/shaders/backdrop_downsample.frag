// 4-tap box 降采样：backdrop 段渲染结果 → L0（KAWASE_BACKDROP_BLUR_PROPOSAL.md §2.1）。
precision mediump float;

in vec2 v_uv;

uniform sampler2D u_texture;
uniform vec2 u_texel; // 1 / 源分辨率

layout(location = 0) out vec4 fragColor;

void main() {
    vec4 c = texture(u_texture, v_uv + vec2(-0.5, -0.5) * u_texel);
    c += texture(u_texture, v_uv + vec2(0.5, -0.5) * u_texel);
    c += texture(u_texture, v_uv + vec2(-0.5, 0.5) * u_texel);
    c += texture(u_texture, v_uv + vec2(0.5, 0.5) * u_texel);
    fragColor = vec4(c.rgb * 0.25, 1.0);
}
