layout (location = 0) in vec3 v_position;
layout (location = 1) flat in int v_batchID;
layout (location = 2) in vec4 v_color;
layout (location = 3) in vec2 v_texCoord;

#include "common/instance.frag.glsl"
#include "common/rounded_clip.glsl"

uniform sampler2D u_texture;

layout (location = 0) out vec4 fragColor;

void main() {
    applyRoundedClip(instanceDisplaySize(), instanceRounding(), v_position);
    fragColor = texture(u_texture, v_texCoord.st);
    fragColor.a *= instanceAlpha();
}
