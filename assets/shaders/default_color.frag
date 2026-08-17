layout (location = 0) in vec3 v_position;
layout (location = 1) flat in int v_batchID;
layout (location = 2) in vec4 v_color;

#ifdef ENABLE_SSBO
struct InstanceData {
    mat4 model;
    vec4 defaultColor;
    vec4 defaultAttr;         //vec2 displaySize, float rounding, float alpha
};
layout (std430, binding = 0) buffer InstanceBuffer {
    InstanceData instances[];
};
#else
uniform vec3 u_displaySize;
uniform float u_rounding;
uniform float u_alpha;
uniform vec4 u_color;
#endif

layout(location = 0) out vec4 fragColor;
void main() {
#ifdef ENABLE_SSBO
    InstanceData instance = instances[v_batchID];
    vec2 displaySize = instance.defaultAttr.xy;
    float rounding = instance.defaultAttr.z;
    float alpha = instance.defaultAttr.w;
    fragColor = instance.defaultColor;
#else
    vec2 displaySize = u_displaySize.xy;
    float rounding = u_rounding;
    float alpha = u_alpha;
    fragColor = u_color;
#endif

    if (rounding > 0.0) {
        vec3 center = vec3(displaySize.x / 2.0 - rounding, displaySize.y / 2.0 - rounding, v_position.z);
        vec3 current = vec3(abs(v_position));
        float distanceSq = dot(current - center, current - center);
        float roundingSq = rounding * rounding;
        if (current.x > center.x && current.y > center.y && distanceSq > roundingSq) {
            discard;
        }
    }
    fragColor.a *= alpha;
}
