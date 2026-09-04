
layout(location = 0) in vec3 a_position;

layout(location = 0) out vec3 v_position;

uniform mat4 u_mvp;
uniform vec2 u_shadowOffset;

void main() {
    gl_Position = u_mvp * vec4(a_position.xy + u_shadowOffset, a_position.z, 1.0);
    v_position = a_position;
}