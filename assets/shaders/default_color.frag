layout (location = 0) in vec3 v_position;
layout (location = 1) flat in int v_batchID;
layout (location = 2) in vec4 v_color;

#include "common/instance.frag.glsl"
#include "common/rounded_clip.glsl"

layout (location = 0) out vec4 fragColor;

void main() {
    applyRoundedClip(instanceDisplaySize(), instanceRounding(), v_position);
    fragColor = instanceColor();
    fragColor.a *= instanceAlpha();
}
