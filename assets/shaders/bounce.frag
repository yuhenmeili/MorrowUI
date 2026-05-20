layout (location = 0) flat in int v_batchID;
layout (location = 1) in vec2 v_texCoord;

#ifdef ENABLE_SSBO
struct InstanceData {
    mat4 model;
    vec4 meshCenter;           //vec3 meshCenter, alpha
    vec4 defaultAttr;         //timeDelta, duration, bounceTimes, scaleRange
};
layout (std430, binding = 0) buffer InstanceBuffer {
    InstanceData instances[];
};
#else
uniform float u_alpha;
#endif
uniform sampler2D u_texture;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = texture2D(u_texture, v_texCoord.st);
    #ifdef ENABLE_SSBO
    InstanceData instance = instances[v_batchID];
    fragColor.a *= instance.meshCenter.w;
    #else
    fragColor.a *= u_alpha;
    #endif
}
