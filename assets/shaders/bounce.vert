#pragma morrow ssbo
layout (location = 0) in vec3 a_position;
layout (location = 1) in float a_batch;
layout (location = 2) in vec2 a_texCoord;

layout (location = 0) flat out int v_batchID;
layout (location = 1) out vec2 v_texCoord;

#include "common/instance.vert.glsl"

const float PI = 3.1415926;

void main() {
    mat4 model = instanceModel();
    vec3 meshCenter = instanceColor1().xyz;
    vec4 animation = instanceExtra();
    float timeDelta = animation.x;
    float duration = animation.y;
    float bounceTimes = animation.z;
    float scaleRange = animation.w;

    v_batchID = instanceBatchID();

    float currentTime = timeDelta * 1000.0;
    float firstStepTime = duration * 1000.0 * 0.5;
    float otherStepTime = firstStepTime * (1.0 / (bounceTimes - 1.0));
    float scale = 1.0;
    if (currentTime <= firstStepTime) {
        scale = currentTime / firstStepTime * (1.0 + scaleRange);
    }
    else {
        float upLimit = 1.0 + scaleRange;
        float downLimit = 1.0 - scaleRange;
        float dispacement = (upLimit - downLimit) / 2.0;
        float offset = dispacement / (bounceTimes - 1.0);
        float currentBounce = floor((currentTime - firstStepTime) / otherStepTime);
        if (currentBounce < (bounceTimes - 1.0)) {
            float currentBounceTime = currentTime - firstStepTime - currentBounce * otherStepTime;
            scale = (dispacement - currentBounce * offset) * cos(currentBounceTime / otherStepTime * PI * 1.5) + 1.0;
        }
    }
    vec3 direct = a_position - meshCenter;
    gl_Position = projectionView * model * vec4(direct * scale + meshCenter, 1.0);
    v_texCoord = a_texCoord;
}
