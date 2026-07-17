layout (location = 0) in vec3 v_position;
layout (location = 1) flat in int v_batchID;
layout (location = 2) in vec4 v_color;
layout (location = 3) in vec2 v_texCoord;

layout (location = 0) out vec4 fragColor;

#ifdef ENABLE_SSBO
struct InstanceData {
    mat4 model;
    vec4 bgColor;
    vec4 defaultAttr;         //vec2 displaySize, float rounding, float alpha
    vec4 textureAttr;         //x=useTexture
};
layout (std430, binding = 0) buffer InstanceBuffer {
    InstanceData instances[];
};
#else
uniform vec3 u_displaySize;
uniform float u_rounding;
uniform float u_alpha;
uniform vec4 u_color;
uniform float u_useTexture;
#endif

uniform sampler2D u_texture;

void main() {
#ifdef ENABLE_SSBO
    InstanceData instance = instances[v_batchID];
    vec2 displaySize = instance.defaultAttr.xy;
    float rounding = instance.defaultAttr.z;
    float alpha = instance.defaultAttr.w;
    vec4 bgColor = instance.bgColor;
    float useTexture = instance.textureAttr.x;
#else
    vec2 displaySize = u_displaySize.xy;
    float rounding = u_rounding;
    float alpha = u_alpha;
    vec4 bgColor = u_color;
    float useTexture = u_useTexture;
#endif
    // 判断是否启用圆角功能
    if (rounding > 0.0) {
        vec3 center = vec3(displaySize.x / 2.0 - rounding, displaySize.y / 2.0 - rounding, v_position.z);
        vec3 current = vec3(abs(v_position));
        float distanceSq = dot(current - center, current - center);
        float roundingSq = rounding * rounding;
        if (current.x > center.x && current.y > center.y && distanceSq > roundingSq) {
            discard;
        }
    }
    // 底图纹理混合
    if (useTexture > 0.5) {
        vec4 texColor = texture(u_texture, v_texCoord);
        fragColor = bgColor * texColor;
    } else {
        fragColor = bgColor;
    }
    fragColor.a *= alpha;
}
