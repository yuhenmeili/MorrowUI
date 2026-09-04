precision highp float;
precision highp int;
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec2 a_texCoord;

layout(location = 0) out vec4 v_color;

uniform mat4 u_mvp;
uniform float u_timeDelta;
uniform vec2 u_imageSize;
uniform float u_duration;
uniform int u_direct;
uniform float u_radiusHighlight;
uniform float u_pointSize;
uniform float u_pointSizeHighlight;
uniform vec4 u_pointColorHighlight;

float calculateDisplacement(float distance)
{
    float currentTime = u_timeDelta * 1000.0;
    float halfSpeed = 0.0;
    float rate = 3.0 / 14.0;

    float acceleratedDuration = u_duration * 1000.0 * rate;
    float acceleratedOffset = distance * rate;
    float acceleration = 2.0 * (acceleratedOffset - halfSpeed * acceleratedDuration) / pow(acceleratedDuration, 2.0);

    float decelerationDuration = u_duration * 1000.0 * (1.0 - rate);
    float decelerationOffset = distance * (1.0 - rate);
    float decelerationStartSpeed = halfSpeed + acceleration * acceleratedDuration;
    float deceleration = (decelerationStartSpeed * decelerationDuration - decelerationOffset) * 2.0 / pow(decelerationDuration, 2.0);

    float displacement = 0.0;
    if (currentTime <= acceleratedDuration) {
        displacement = halfSpeed * currentTime + 0.5 * acceleration * pow(currentTime, 2.0);
    }
    else {
        float decelerationTime = currentTime - acceleratedDuration;
        displacement += acceleratedOffset;
        displacement += decelerationStartSpeed * decelerationTime - 0.5 * deceleration * pow(decelerationTime, 2.0);
    }
    return displacement;
}

void main()
{
    vec3 currentPosition = vec3(a_position.xyz);
    float startPointY = u_imageSize.y / 2.0 * float(u_direct);
    vec3 startPoint = vec3(a_position.x, startPointY, a_position.z);
    vec3 currentToStartDirect = currentPosition - startPoint;
    vec3 currentToStartNormal = normalize(currentToStartDirect);
    vec3 currentToYaxis = vec3(currentPosition.x,0.0,a_position.z);
    vec3 currentToYaxisNormal = normalize(currentToYaxis);
    float distance = u_direct != 0 ? u_imageSize.y : u_imageSize.y / 2.0;
    float displacement = u_duration == 0.0 ? distance : calculateDisplacement(distance);
    float offset = abs(a_position.y - startPointY);
    float XFlag = 1.0 - smoothstep(0.0, u_imageSize.x, abs(a_position.x));//最小也偏移2个单位
    float YFlag = 1.0 - smoothstep(0.0, u_radiusHighlight, abs(offset - displacement));
    float factor = abs(offset - displacement) == 0.0 ? 0.0 : ((offset - displacement) / abs(offset - displacement));//控制方向
    bool ignoreY = u_direct == 0 && a_position.y == 0.0;
    bool isEnd = abs(abs(a_position.y) - u_imageSize.y / 2.0) < 20.0;//是否是两端
    float YAttenuation = 1.0 - (u_direct == 0 ? smoothstep(0.0, u_imageSize.y / 2.0, abs(a_position.y)) : 0.0);//Y方向偏移衰减
    currentPosition += ignoreY ? vec3(0.0) : currentToStartNormal * YFlag * factor * 13.0 * YAttenuation * (isEnd ? 0.1 : 1.0);
    currentPosition += currentToYaxisNormal * XFlag * YFlag * 4.0;
    gl_Position = u_mvp * vec4(currentPosition.xyz, 1.0);
    float pointSize = u_pointSize + YFlag * u_pointSizeHighlight * (isEnd ? 0.2 : 1.0);
    gl_PointSize = pointSize;

    bool setHighlightColor = length(u_pointColorHighlight) != 0.0 && YFlag > 0.5;
    v_color = setHighlightColor ? u_pointColorHighlight * YFlag : a_color;
}