#version 460
precision mediump float;
precision mediump int;

// uniform sampler2D u_texture;
uniform float u_timeDelta;
uniform float u_duration;
uniform vec2 u_displaySize;
uniform float u_flowingLightLength;
uniform vec4 u_flowingLightColor;
uniform float u_flowingLightThickness;
uniform float u_alpha;

in vec2 v_texCoord;
in vec3 v_position;

layout(location = 0) out vec4 fragColor;

void main()
{
    // vec4 result = vec4(texture2D(u_texture, v_texCoord.xy).a);
    vec4 result = vec4(0.0);
    float progress = u_timeDelta/u_duration;
    vec3 startPoint = vec3(u_displaySize.x/2.0,u_displaySize.y/2.0, 0.0);
    float widthDistance = abs(abs(v_position.x) - u_displaySize.x/2.0);
    float heightDistance = abs(abs(v_position.y) - u_displaySize.y/2.0);
    vec3 correctionalPosition = v_position;
    if(widthDistance <= u_flowingLightThickness){
        correctionalPosition.x = (correctionalPosition.x < 0.0 ? -1.0 : 1.0) * u_displaySize.x/2.0;
    }
    if(heightDistance <= u_flowingLightThickness){
        correctionalPosition.y = (correctionalPosition.y < 0.0 ? -1.0 : 1.0) * u_displaySize.y/2.0;
    }
    vec3 crossVector = cross(startPoint,correctionalPosition);
    vec3 currentDir = correctionalPosition - startPoint;
    float halfDistance = u_displaySize.x + u_displaySize.y;
    float totalDistance = halfDistance * 2.0 + u_flowingLightLength;
    float headDistance = progress * totalDistance;
    float tailDistance = headDistance - u_flowingLightLength;
    float currentDistance = abs(currentDir.x) + abs(currentDir.y);
    bool isInFlowing = false;
    if(widthDistance <= u_flowingLightThickness || heightDistance <= u_flowingLightThickness){
        if(crossVector.z >= 0.0){
            currentDistance = halfDistance * 2.0 - currentDistance;
        }
        isInFlowing = currentDistance >= tailDistance && currentDistance <= headDistance;
    }
    if(isInFlowing){
        float leftDistance = currentDistance - tailDistance;
        float alpha = leftDistance / u_flowingLightLength;
        alpha = max(alpha, 0.3);
        result = vec4(u_flowingLightColor.xyz,alpha);
    }
    fragColor = result;
    fragColor.a *= u_alpha;
}
