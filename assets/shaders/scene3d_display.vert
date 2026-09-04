precision mediump float;

layout(location = 0) in vec3 a_position;
layout(location = 2) in vec2 a_texCoord;

uniform mat4 u_projectionView;
uniform mat4 u_model;

out vec2 v_uv;

void main() {
    v_uv        = a_texCoord;
    gl_Position = u_projectionView * u_model * vec4(a_position, 1.0);
}

