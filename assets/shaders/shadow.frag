
layout (location = 0) in vec3 v_position;

uniform vec4 u_shadowColor;
uniform vec3 u_displaySize;
uniform vec2 u_shadowOffsetFade;
uniform float u_alpha;

layout (location = 0) out vec4 fragColor;

void main() {
    vec3 halfSize = u_displaySize / 2.0;
    float distanceToXEdge = halfSize.x - abs(v_position.x);
    float distanceToYEdge = halfSize.y - abs(v_position.y);
    float distance = min(distanceToXEdge, distanceToYEdge);
    bool isXEdge = (distance == distanceToXEdge);
    float shadowAlpha = mix(smoothstep(0.0, u_shadowOffsetFade.y, distance), smoothstep(0.0, u_shadowOffsetFade.x, distance), float(isXEdge));
    fragColor = u_shadowColor;
    fragColor.a *= u_alpha * shadowAlpha;
}