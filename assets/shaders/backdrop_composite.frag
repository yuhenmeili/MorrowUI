// 回屏背景恢复拷贝：把 backdrop RT 的内容以全屏面片写回默认帧缓冲
//（KAWASE_BACKDROP_BLUR_PROPOSAL.md §5.1 阶段 3c）。RGBA 原样恢复——
// backdrop RT 与默认帧缓冲经历相同的清屏与 backdrop 段绘制，其 alpha
// 就是"无模糊路径"本应写出的值：不透明窗口下为 1，透明窗口（如 AVM 的
// 透明清屏 + 渐变 alpha 背景图）下保留渐变透明。强制 alpha=1 会把透明
// 区域变成不透明黑（AIM 现象二）。模糊面片随后在不透明替换语义下覆盖
// 各自矩形，不受此处 alpha 影响。
precision mediump float;

in vec2 v_uv;

uniform sampler2D u_texture;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = texture(u_texture, v_uv);
}
