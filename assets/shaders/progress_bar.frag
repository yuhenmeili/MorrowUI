layout (location = 0) in vec3 v_position;
layout (location = 1) flat in int v_batchID;
layout (location = 2) in vec4 v_color;
layout (location = 3) in vec2 v_texCoord;

#ifdef ENABLE_SSBO
struct InstanceData {
    mat4 model;
    vec4 trackColor;
    vec4 fillColor;
    vec4 defaultAttr;   //vec2 displaySize, float rounding, float alpha
    vec4 stateAttr;     //float progress, float direction, float useTrackTexture, float useFillTexture
};
layout (std430, binding = 0) buffer InstanceBuffer {
    InstanceData instances[];
};
#else
uniform vec3 u_displaySize;
uniform float u_rounding;
uniform float u_alpha;
uniform float u_progress;
uniform float u_direction;
uniform vec4 u_trackColor;
uniform vec4 u_fillColor;
uniform float u_useTrackTexture;
uniform float u_useFillTexture;
#endif

uniform sampler2D u_trackTexture;
uniform sampler2D u_fillTexture;

layout (location = 0) out vec4 fragColor;
void main() {
#ifdef ENABLE_SSBO
    InstanceData instance = instances[v_batchID];
    vec2 displaySize = instance.defaultAttr.xy;
    float rounding = instance.defaultAttr.z;
    float alpha = instance.defaultAttr.w;
    vec4 trackColor = instance.trackColor;
    vec4 fillColor = instance.fillColor;
    float progress = instance.stateAttr.x;
    float direction = instance.stateAttr.y;
    float useTrackTexture = instance.stateAttr.z;
    float useFillTexture = instance.stateAttr.w;
#else
    vec2 displaySize = u_displaySize.xy;
    float rounding = u_rounding;
    float alpha = u_alpha;
    vec4 trackColor = u_trackColor;
    vec4 fillColor = u_fillColor;
    float progress = u_progress;
    float direction = u_direction;
    float useTrackTexture = u_useTrackTexture;
    float useFillTexture = u_useFillTexture;
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

    // 归一化填充坐标（0 = 起点，1 = 终点），与 MRSlider 的屏幕坐标映射保持一致
    vec2 halfSize = displaySize * 0.5;
    float coord;
    if (direction < 0.5) {
        coord = (v_position.x + halfSize.x) / displaySize.x;   // Left -> Right
    } else if (direction < 1.5) {
        coord = (halfSize.x - v_position.x) / displaySize.x;   // Right -> Left
    } else if (direction < 2.5) {
        coord = (v_position.y + halfSize.y) / displaySize.y;   // Bottom -> Top
    } else {
        coord = (halfSize.y - v_position.y) / displaySize.y;   // Top -> Bottom
    }

    vec4 color;
    if (coord <= progress) {
        color = fillColor;
        if (useFillTexture > 0.5) {
            color *= texture(u_fillTexture, v_texCoord);
        }
    } else {
        color = trackColor;
        if (useTrackTexture > 0.5) {
            color *= texture(u_trackTexture, v_texCoord);
        }
    }
    fragColor = color;
    fragColor.a *= alpha;
}
