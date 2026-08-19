layout(location = 0) in vec3 a_position;
layout(location = 2) in vec4 a_color;
layout(location = 3) in vec2 a_texCoord;

layout(location = 0) out vec4 v_color;
layout(location = 1) out vec2 v_texCoord;

uniform mat4 u_mvp;
uniform float u_time;
uniform float u_lifetime;
uniform float u_particleSize;
uniform vec2 u_emissionSize;
uniform vec2 u_velocityMin;
uniform vec2 u_velocityMax;
uniform vec2 u_gravity;
uniform vec4 u_startColor;
uniform vec4 u_endColor;

void main() {
    float phase = fract(u_time / u_lifetime + a_color.w);
    float age = phase * u_lifetime;
    vec2 origin = (a_color.xy - vec2(0.5)) * u_emissionSize;
    vec2 velocity = mix(u_velocityMin, u_velocityMax, vec2(a_color.z, a_color.x));
    vec2 motion = origin + velocity * age + 0.5 * u_gravity * age * age;
    float sizeVariation = mix(0.65, 1.35, a_color.y);
    vec2 corner = a_position.xy * u_particleSize * sizeVariation;
    gl_Position = u_mvp * vec4(motion + corner, 0.0, 1.0);
    v_color = mix(u_startColor, u_endColor, phase);
    v_texCoord = a_texCoord;
}
