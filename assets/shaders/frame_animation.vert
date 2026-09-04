
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec2 a_texCoord;

layout(location = 0) out vec3 v_position;
layout(location = 1) out vec4 v_color;
layout(location = 2) out vec2 v_texCoord;

uniform mat4 u_mvp;

void main()
{
    gl_Position = u_mvp * vec4(a_position.xyz, 1.0);
    v_position = a_position;
    v_color = a_color;
    v_texCoord = a_texCoord;
}
