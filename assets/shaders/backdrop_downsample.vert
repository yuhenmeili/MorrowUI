// 降采样 pass 全屏面片顶点着色器（KAWASE_BACKDROP_BLUR_PROPOSAL.md §5.6）。
// 顶点坐标直接是裁剪空间（-1..1），不经过相机矩阵，也不依赖全局 UBO。
precision highp float;

layout(location = 0) in vec3 a_position;
layout(location = 2) in vec2 a_texCoord;

out vec2 v_uv;

void main() {
    v_uv = a_texCoord;
    gl_Position = vec4(a_position, 1.0);
}
