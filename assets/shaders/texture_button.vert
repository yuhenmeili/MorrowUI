layout (location = 0) in vec3 a_position;
layout (location = 1) in float a_batch;
layout (location = 2) in vec2 a_texCoord;

layout (location = 0) flat out int v_batchID;
layout (location = 1) out vec2 v_texCoord;

layout (std140) uniform Global {
    mat4 projectionView;
};

#ifdef ENABLE_SSBO
struct InstanceData {
    mat4 model;
    vec4 defaultAttr;         // alpha, 0, 0, 0
};
layout (std430, binding = 0) buffer InstanceBuffer {
    InstanceData instances[];
};
#else
uniform mat4 u_model;
#endif

void main() {
#ifdef ENABLE_SSBO
    int batchID = int(floor(a_batch + 0.1));
    InstanceData instance = instances[batchID];
    gl_Position = projectionView * instance.model * vec4(a_position, 1.0);
    v_batchID = batchID;
#else
    gl_Position = projectionView * u_model * vec4(a_position, 1.0);
    v_batchID = 0;
#endif
    v_texCoord = a_texCoord;
}
