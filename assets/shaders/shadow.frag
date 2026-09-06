precision highp float;

layout (location = 0) in vec3 v_position;

uniform vec4 u_shadowColor;
uniform vec3 u_displaySize;
uniform float u_shadowBlur;
uniform float u_shadowSpread;
uniform float u_shadowRounding;
uniform float u_alpha;

layout (location = 0) out vec4 fragColor;

float sdRoundedBox(vec2 position, vec2 halfSize, float rounding) {
    float clampedRounding = min(rounding, min(halfSize.x, halfSize.y));
    vec2 q = abs(position) - halfSize + vec2(clampedRounding);
    return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - clampedRounding;
}

void main() {
    vec2 halfSize = max(u_displaySize.xy * 0.5, vec2(0.5));
    float blur = max(u_shadowBlur, 0.001);
    float rounding = max(u_shadowRounding, 0.0);

    float dShadow = sdRoundedBox(v_position.xy, halfSize + vec2(u_shadowSpread), rounding);
    float t = max(dShadow, 0.0) / blur;
    float falloff = exp(-t * t * 3.0);

    float dOwner = sdRoundedBox(v_position.xy, halfSize, rounding);
    float outerOnly = smoothstep(-0.5, 0.5, dOwner);

    fragColor = vec4(u_shadowColor.rgb, u_shadowColor.a * u_alpha * falloff * outerOnly);
}
