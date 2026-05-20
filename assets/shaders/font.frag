layout (location = 0) in vec2 v_texCoord;
layout (location = 1) flat in int v_batchID;

#ifdef ENABLE_SSBO
struct InstanceData {
    mat4 model;
    vec4 fontColor;
    vec4 fontAttr;
};
layout (std430, binding = 0) buffer InstanceBuffer {
    InstanceData instances[];
};
#else

uniform vec4 u_fontColor;
uniform float u_alpha;
#endif
uniform sampler2D u_texture;

layout (location = 0) out vec4 fragColor;

void main() {
#ifdef ENABLE_SSBO
    InstanceData instance = instances[v_batchID];
    vec4 fontColor = instance.fontColor;
    float alpha = instance.fontAttr.x;
#else
    vec4 fontColor = u_fontColor;
    float alpha = u_alpha;
#endif
    vec4 source = texture(u_texture, v_texCoord.st);
    float coverage = source.r;
    fragColor = vec4(fontColor.rgb, coverage * fontColor.a * alpha);
}
