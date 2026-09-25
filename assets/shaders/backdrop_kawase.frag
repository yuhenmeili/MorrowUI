// Kawase 模糊单 pass：中心 4 权重 + 四角各 1 权重取平均（KAWASE_BACKDROP_BLUR_PROPOSAL.md §2.1）。
// 采样偏移随 pass 序号递增（0.5 / 1.5 / ...），多 pass 叠加近似大高斯。
precision mediump float;

in vec2 v_uv;

uniform sampler2D u_texture;
uniform vec2 u_texel;  // 1 / 当前层级分辨率
uniform float u_offset; // 本 pass 的采样偏移（像素）

layout(location = 0) out vec4 fragColor;

void main() {
    vec3 c = texture(u_texture, v_uv).rgb * 4.0;
    c += texture(u_texture, v_uv + vec2(u_offset, u_offset) * u_texel).rgb;
    c += texture(u_texture, v_uv + vec2(-u_offset, u_offset) * u_texel).rgb;
    c += texture(u_texture, v_uv + vec2(u_offset, -u_offset) * u_texel).rgb;
    c += texture(u_texture, v_uv + vec2(-u_offset, -u_offset) * u_texel).rgb;
    fragColor = vec4(c / 8.0, 1.0);
}
