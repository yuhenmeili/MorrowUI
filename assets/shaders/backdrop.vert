// 模糊面片顶点着色器：采样共享模糊链的玻璃面板面片
//（KAWASE_BACKDROP_BLUR_PROPOSAL.md §5.1 阶段 3c / §5.6 / S3）。
//
// 每顶点数据由 BackdropBlurManager 在 CPU 侧按面片烘焙：
//   a_position = 世界空间角点（属主 Transform 世界矩阵已烘焙）
//   a_texCoord = 该角点对应的 backdrop 采样 UV（屏幕矩形映射到 RT）
//   a_normal   = (对象空间 x, 对象空间 y, 圆角半径)  —— 复用 Normal 语义
//   a_tangent  = (displaySize.x, displaySize.y, 0, 双层插值因子) —— 复用 Tangent 语义
//   a_color    = tint 混色（rgb + 混色强度）
precision highp float;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec2 a_texCoord;
layout(location = 3) in vec3 a_normal;
layout(location = 4) in vec4 a_tangent;

uniform mat4 u_projectionView;

out vec2 v_uv;
out vec4 v_tint;
out vec2 v_objPos;
out vec3 v_sizeRound; // xy = displaySize, z = rounding
out float v_blend;    // 双层插值因子（0 = 单层采样）

void main() {
    v_uv = a_texCoord;
    v_tint = a_color;
    v_objPos = a_normal.xy;
    v_sizeRound = vec3(a_tangent.xy, a_normal.z);
    v_blend = a_tangent.w;
    gl_Position = u_projectionView * vec4(a_position, 1.0);
}
