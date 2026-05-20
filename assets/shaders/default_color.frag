layout(location = 0) flat in int v_batchID;
layout(location = 1) in vec4 v_color;

#ifdef ENABLE_SSBO
struct InstanceData {
    mat4 model;
    vec4 defaultColor;
    vec4 defaultAttr;         //alpha, 0, 0, 0
};
layout (std430, binding = 0) buffer InstanceBuffer {
    InstanceData instances[];
};
#else
uniform float u_alpha;
uniform vec4 u_color;
#endif

layout(location = 0) out vec4 fragColor;
void main() {
#ifdef ENABLE_SSBO
    InstanceData instance = instances[v_batchID];
    fragColor = instance.defaultColor;
    fragColor.a *= instance.defaultAttr.x;
#else
    fragColor = u_color;
    fragColor.a *= u_alpha;
#endif
}
