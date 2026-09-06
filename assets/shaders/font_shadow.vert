layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_texCoord;

layout (location = 0) out vec2 v_texCoord;

uniform mat4 u_mvp;
uniform vec2 u_shadowOffset;    //对象空间偏移（px），由组件做屏幕方向换算

void main() {
    gl_Position = u_mvp * vec4(a_position.xy + u_shadowOffset, a_position.z, 1.0);
    v_texCoord = a_texCoord;
}
