#version 460
precision mediump float;
precision mediump int;

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec4 v_color;
layout(location = 2) in vec2 v_texCoord;

uniform sampler2D u_texture;
uniform float u_alpha;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec4 base_color = texture2D(u_texture, v_texCoord.st);
    fragColor = v_color * base_color;
    fragColor.a *= u_alpha;
}
