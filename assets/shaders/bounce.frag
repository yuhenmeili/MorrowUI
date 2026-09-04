layout (location = 0) flat in int v_batchID;
layout (location = 1) in vec2 v_texCoord;

#include "common/instance.frag.glsl"

uniform sampler2D u_texture;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = texture(u_texture, v_texCoord.st);
    fragColor.a *= instanceColor1().w;
}
