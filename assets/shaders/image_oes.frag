#extension GL_OES_EGL_image_external : require
#extension GL_OES_EGL_image_external_essl3 : require
precision mediump float;
precision mediump int;

layout (location = 0) in vec3 v_position;
layout (location = 1) flat in int v_batchID;
layout (location = 2) in vec4 v_color;
layout (location = 3) in vec2 v_texCoord;

#include "common/instance.frag.glsl"
#include "common/rounded_clip.glsl"

uniform samplerExternalOES u_texture;

layout (location = 0) out vec4 fragColor;

void main() {
    applyRoundedClip(instanceDisplaySize(), instanceRounding(), v_position);
    vec4 base_color = texture(u_texture, v_texCoord.st);
    vec4 color = v_color * base_color;
    fragColor = color;
    fragColor.a *= instanceAlpha();
}
