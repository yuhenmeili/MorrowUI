// tint 降级面片顶点着色器：模糊关闭时的纯色半透明面板（Shadow 同款模式，
// 走普通通道 / renderStandardBatch，u_mvp 由批次管理器逐对象设置）。
precision highp float;

layout(location = 0) in vec3 a_position;

uniform mat4 u_mvp;

out vec3 v_position;

void main() {
    v_position = a_position;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
