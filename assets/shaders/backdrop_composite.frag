// 回屏背景恢复拷贝：把 backdrop RT 的内容以全屏面片写回默认帧缓冲
//（KAWASE_BACKDROP_BLUR_PROPOSAL.md §5.1 阶段 3c）。alpha 强制 1，
// 避免 backdrop 段半透明内容污染默认帧缓冲的 alpha 通道（§8.3）。
precision mediump float;

in vec2 v_uv;

uniform sampler2D u_texture;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(texture(u_texture, v_uv).rgb, 1.0);
}
