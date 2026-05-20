#version 460
precision mediump float;
precision mediump int;

layout (location = 0) in vec4 v_color;

uniform float u_alpha;

layout (location = 0) out vec4 fragColor;
void main()
{
    fragColor = v_color;
    fragColor.a *= u_alpha;
}