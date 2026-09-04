layout (location = 0) in vec2 v_texCoord;
layout (location = 1) flat in int v_batchID;

#include "common/instance.frag.glsl"

uniform sampler2D u_texture;

layout (location = 0) out vec4 fragColor;

void main() {
    vec4 fontColor = instanceColor();
    vec4 source = texture(u_texture, v_texCoord.st);
    float coverage = source.r;
    fragColor = vec4(fontColor.rgb, coverage * fontColor.a * instanceAlpha());
}
