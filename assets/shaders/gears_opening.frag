#version 460
precision highp float;
precision highp int;
layout(location = 0) in vec2 v_texCoord;

uniform float u_timeDelta;
uniform vec2 u_imageSize;
uniform float u_duration;
uniform bool u_forward;
uniform float u_blurRadius;
uniform sampler2D u_texture;
uniform float u_alpha;

layout(location = 0) out vec4 fragColor;

float calculateDisplacement(float distance)
{
    float currentTime = u_timeDelta * 1000.0;
    float halfSpeed = distance / (u_duration * 1000.0) * 0.5;
    float rate = 1.0 / 5.0;

    float acceleratedDuration = u_duration * 1000.0 * rate;
    float acceleratedOffset = distance  * rate;
    float acceleration = 2.0 * (acceleratedOffset - halfSpeed * acceleratedDuration)/pow(acceleratedDuration,2.0);

    float decelerationDuration = u_duration * 1000.0 * (1.0 - rate);
    float decelerationOffset = distance * (1.0 - rate);
    float decelerationStartSpeed = halfSpeed + acceleration * acceleratedDuration;
    float deceleration = (decelerationStartSpeed * decelerationDuration - decelerationOffset) * 2.0 / pow(decelerationDuration, 2.0);

    float displacement = 0.0;
    if(currentTime <= acceleratedDuration){
        displacement = halfSpeed * currentTime + 0.5 * acceleration * pow(currentTime, 2.0);
    }
    else{
        float decelerationTime = currentTime - acceleratedDuration;
        displacement += acceleratedOffset;
        displacement += decelerationStartSpeed * decelerationTime - 0.5 * deceleration * pow(decelerationTime, 2.0);
    }
    return displacement;
}

void main()
{
    fragColor = texture2D(u_texture, v_texCoord);
    vec2 uvCenter = vec2(0.5, 0.5);
    float distance = 0.5 * u_imageSize.y + u_blurRadius;
    float displacement = u_duration == 0.0 ? distance : calculateDisplacement(distance);
    vec2 currentToCenterUV = v_texCoord - uvCenter;
    vec2 currentPositionToCenter = currentToCenterUV * u_imageSize;
    float currentHalfHeight = u_forward ? displacement : (distance - displacement);
    float yAlpha = smoothstep(0.0, u_blurRadius, currentHalfHeight - abs(currentPositionToCenter.y));
    fragColor.a *= yAlpha * u_alpha;
}