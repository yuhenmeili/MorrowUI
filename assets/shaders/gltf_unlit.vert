#version 460
precision mediump float;

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texCoord;

layout(std140) uniform Scene3DFrame {
    mat4 u_projectionView;
    vec4 u_cameraWorldPos;
    vec4 u_sunDirection;
    vec4 u_sunColorIntensity;
    vec4 u_ambientColorIntensity;
    vec4 u_iblParams;
    ivec4 u_frameFlags;
    ivec4 u_specularMipInfo[8];
};

layout(std140) uniform Scene3DDraw {
    mat4 u_modelMatrix;
};

out vec2 v_uv;

void main() {
    v_uv = a_texCoord;
    gl_Position = u_projectionView * u_modelMatrix * vec4(a_position, 1.0);
}