layout (location = 0) in vec3 v_position;
layout (location = 1) flat in int v_batchID;
layout (location = 2) in vec4 v_color;
layout (location = 3) in vec2 v_texCoord;

#ifdef ENABLE_SSBO
struct InstanceData {
    mat4 model;
    vec4 displaySize;
    vec4 imageAttr;
};
layout (std430, binding = 0) buffer InstanceBuffer {
    InstanceData instances[];
};
#else
uniform float u_alpha;
#endif
uniform sampler2D u_texture;

layout (location = 0) out vec4 fragColor;

void main() {
#ifdef ENABLE_SSBO
    InstanceData instance = instances[v_batchID];
    float alpha = instance.imageAttr.y;
#else
    float alpha = u_alpha;
#endif

    vec4 source = texture(u_texture, v_texCoord.st);
    float value = source.r;
    fragColor = vec4(vec3(value), alpha);
}

