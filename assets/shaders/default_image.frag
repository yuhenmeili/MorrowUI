layout (location = 0) flat in int v_batchID;
layout (location = 1) in vec2 v_texCoord;

#ifdef ENABLE_SSBO
struct InstanceData {
    mat4 model;
    vec4 defaultAttr;         //alpha, 0, 0, 0
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
    fragColor = texture(u_texture, v_texCoord.st);
#ifdef ENABLE_SSBO
    InstanceData instance = instances[v_batchID];
    fragColor.a *= instance.defaultAttr.x;
#else
    fragColor.a *= u_alpha;
#endif
}
