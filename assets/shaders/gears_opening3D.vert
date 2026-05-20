#version 460
precision highp float;
precision highp int;
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec4 a_color;
layout (location = 2) in vec2 a_texCoord;

layout (location = 0) out vec4 v_color;
layout (location = 1) out vec3 v_position;

uniform mat4 u_mvp;
uniform float u_pointSize;

void main()
{
    gl_Position = u_mvp * vec4(a_position.xyz, 1.0);
    gl_PointSize = u_pointSize;
    v_color = a_color;
    v_position = a_position;
}