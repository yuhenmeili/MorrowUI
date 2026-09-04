precision highp float;
precision highp int;
layout(location = 0) in vec2 v_texCoord;

uniform float u_timeDelta;
uniform vec2 u_imageSize;
uniform float u_duration;
uniform float u_circleRadius;
uniform float u_circleDelay;
uniform vec4 u_circleColor;
uniform float u_gridGap;
uniform float u_gridResetTime;//ms
uniform vec4 u_gridHalfWidthParameter;
uniform vec4 u_gridColor;
uniform float u_alpha;

layout(location = 0) out vec4 fragColor;
float random (vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233)))* 43758.5453123);
}

vec4 getCircleColor(float time,float dist)
{
    float speed = u_circleRadius/((u_duration - u_circleDelay) * 1000.0);
    float displacement = time * speed;
    displacement = displacement > u_circleRadius ? u_circleRadius : displacement;
    float step = 1.0 - step(displacement, dist);
    float alpha = 1.0 - smoothstep(0.0, u_circleRadius, displacement);
    vec4 blendColor = u_circleColor * step;
    blendColor.a = alpha;
    return blendColor;
}

void main()
{
    vec4 textureColor = vec4(0.0);
    float currentTime = u_timeDelta * 1000.0;
    if (currentTime > u_duration * 1000.0) {
        fragColor = textureColor;
    }
    else {
        vec2 uvCenter = vec2(0.5, 0.5);
        vec2 center = vec2(u_imageSize.x * 0.5, u_imageSize.y * 0.5);
        vec2 currentPosition = vec2(u_imageSize.x * v_texCoord.x, u_imageSize.y * v_texCoord.y);
        float distance = length(currentPosition - center);
        vec4 blendColor = getCircleColor(currentTime, distance);
        float delayTime = u_circleDelay * 1000.0;
        vec4 secBlendColor = currentTime > delayTime ? getCircleColor(currentTime - delayTime, distance) : vec4(0.0);

        if (length(blendColor.xyz) > 0.0 || length(secBlendColor.xyz) > 0.0) {
            fragColor = blendColor + secBlendColor;
            float halfGap = u_gridGap * 0.5;
            float tag = floor(currentTime / (u_gridResetTime * 1000.0));
            float xGridNum = mod(currentPosition.x, u_gridGap) <= halfGap ? 0.0 : 1.0;
            float yGridNum = mod(currentPosition.y, u_gridGap) <= halfGap ? 0.0 : 1.0;
            float xPosition = (floor(currentPosition.x / u_gridGap) + xGridNum) * u_gridGap;
            float yPosition = (floor(currentPosition.y / u_gridGap) + yGridNum) * u_gridGap;
            float xDist = abs(currentPosition.x - xPosition);
            float yDist = abs(currentPosition.y - yPosition);
            vec2 gridCenterUV = vec2(xPosition / u_imageSize.x, yPosition / u_imageSize.y);
            float scale = random(gridCenterUV + vec2(tag, tag)) * u_gridHalfWidthParameter.x;
            if (xDist < scale && yDist < scale) {
                if (scale > u_gridHalfWidthParameter.y) {
                    if (xDist > u_gridHalfWidthParameter.z || yDist > u_gridHalfWidthParameter.z) {
                        fragColor += u_gridColor;
                    }
                    if (random(gridCenterUV) > 0.8 && xDist < u_gridHalfWidthParameter.w && yDist < u_gridHalfWidthParameter.w) {
                        fragColor += u_gridColor;
                    }
                }
                else {
                    fragColor += u_gridColor;
                }
            }
        }
        else {
            fragColor = textureColor;
        }
    }
    fragColor.a *= u_alpha;
}