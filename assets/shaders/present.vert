// 上屏贴图 pass 顶点着色器（多屏镜像 PresentStage）。
// 裁剪空间全屏四边形（-1..1）经 u_scale/u_offset 缩放平移到目标窗口的
// letterbox 内容区域，不经过相机矩阵，也不依赖全局 UBO。
precision highp float;

layout(location = 0) in vec3 a_position;
layout(location = 2) in vec2 a_texCoord;

uniform vec2 u_scale;    // 内容→窗口 NDC 半宽/半高缩放比
uniform vec2 u_offset;   // 内容区域中心的 NDC 偏移

out vec2 v_uv;

void main() {
    v_uv = a_texCoord;
    gl_Position = vec4(a_position.xy * u_scale + u_offset, a_position.z, 1.0);
}
