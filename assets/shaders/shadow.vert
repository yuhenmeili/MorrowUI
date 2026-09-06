precision highp float;

layout(location = 0) in vec3 a_position;

layout(location = 0) out vec3 v_position;

uniform mat4 u_mvp;
uniform vec2 u_shadowOffset;
uniform vec3 u_displaySize;     //xy = 属主尺寸
uniform float u_shadowExpand;   //几何外扩（px）= blur + spread，容纳衰减区

void main() {
    // 几何外扩：blur 使阴影足迹超出属主矩形 expand 像素，quad 需等比放大，
    // 否则衰减区被 quad 边缘截断
    vec2 halfSize = max(u_displaySize.xy * 0.5, vec2(0.5));
    vec2 scale = (halfSize + vec2(max(u_shadowExpand, 0.0))) / halfSize;
    vec2 offsetPosition = a_position.xy * scale + u_shadowOffset;
    gl_Position = u_mvp * vec4(offsetPosition, a_position.z, 1.0);
    // 传外扩后的对象空间坐标（阴影中心为原点），SDF 在片元阶段求值
    v_position = vec3(a_position.xy * scale, a_position.z);
}
