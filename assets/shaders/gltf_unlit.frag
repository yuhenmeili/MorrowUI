precision mediump float;

in vec2 v_uv;

uniform sampler2D u_baseColorTexture;

layout(std140) uniform Scene3DMaterial {
    vec4 u_baseColorFactor;
    vec4 u_emissiveFactor;
    vec4 u_materialParams;
    vec4 u_alphaParams;
    ivec4 u_materialFlags0;
    ivec4 u_materialFlags1;
};

out vec4 fragColor;

void main() {
    vec4 color = u_baseColorFactor;
    if (u_materialFlags0.z != 0) {
        color *= texture(u_baseColorTexture, v_uv);
    }
    fragColor = color;
}