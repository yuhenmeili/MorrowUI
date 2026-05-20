#version 460
precision mediump float;
precision mediump int;

layout(location = 0) in vec3 a_position;

layout(location = 0) out vec3 v_position;

uniform mat4 u_mvp;

void main()
{
    gl_Position = u_mvp * vec4(a_position.xyz, 1.0);
    v_position = a_position;
}