#version 460
precision mediump float;
precision mediump int;

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec2 v_texCoord;

layout (location = 0) out vec4 fragColor;

//uniform vec3 u_displaySize;
//uniform float u_rounding;
uniform float u_alpha;
uniform sampler2D u_texture;

void main()
{
    // 判断是否启用圆角功能
//    if (u_rounding > 0.0) {
//        vec3 center = vec3(u_displaySize.x / 2.0 - u_rounding, u_displaySize.y / 2.0 - u_rounding, v_position.z);
//        vec3 current = vec3(abs(v_position));
//        float distanceSq = dot(current - center, current - center);
//        float roundingSq = u_rounding * u_rounding;
//        if (current.x > center.x && current.y > center.y && distanceSq > roundingSq) {
//            discard;
//        }
//    }
    fragColor = vec4(texture(u_texture, v_texCoord));
    fragColor.a *= u_alpha;
}
