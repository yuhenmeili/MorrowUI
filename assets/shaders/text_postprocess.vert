layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec2 a_texCoord;
out vec2 v_texCoord;
out vec4 v_color;

uniform mat4 u_mvp;

void main()
{
    gl_Position = u_mvp * vec4(a_position.xyz, 1.0);
    v_texCoord = a_texCoord;
    v_color = a_color;
}