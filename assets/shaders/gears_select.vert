layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;

layout(location = 0) out vec4 v_color;

uniform mat4 u_mvp;
uniform float u_pointSize;

void main()
{
    gl_Position = u_mvp * vec4(a_position.xyz, 1.0);
    gl_PointSize = u_pointSize;
    v_color = a_color;
}