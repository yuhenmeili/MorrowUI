layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_texCoord;

layout(location = 0) out vec4 fragColor;

void main() {
    vec2 centered = v_texCoord * 2.0 - 1.0;
    float distanceToCenter = length(centered);
    if (distanceToCenter > 1.0) {
        discard;
    }
    float softEdge = 1.0 - smoothstep(0.55, 1.0, distanceToCenter);
    fragColor = vec4(v_color.rgb, v_color.a * softEdge);
}
