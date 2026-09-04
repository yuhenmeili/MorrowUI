layout (location = 0) in vec3 v_position;
layout (location = 1) flat in int v_batchID;
layout (location = 2) in vec4 v_color;
layout (location = 3) in vec2 v_texCoord;

#include "common/instance.frag.glsl"
#include "common/rounded_clip.glsl"

uniform sampler2D u_trackTexture;
uniform sampler2D u_fillTexture;

layout (location = 0) out vec4 fragColor;

void main() {
    applyRoundedClip(instanceDisplaySize(), instanceRounding(), v_position);

    vec4 trackColor = instanceColor();
    vec4 fillColor = instanceColor1();
    vec4 state = instanceState();
    float progress = state.x;
    float direction = state.y;
    float useTrackTexture = state.z;
    float useFillTexture = state.w;
    vec2 displaySize = instanceDisplaySize();

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
    fragColor.a *= instanceAlpha();
}
