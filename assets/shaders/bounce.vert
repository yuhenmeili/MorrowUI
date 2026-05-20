layout (location = 0) in vec3 a_position;
layout (location = 1) in float a_batch;
layout (location = 2) in vec2 a_texCoord;

layout (location = 0) flat out int v_batchID;
layout (location = 1) out vec2 v_texCoord;

layout (std140) uniform Global {
    mat4 projectionView;
};

#ifdef ENABLE_SSBO
struct InstanceData {
    mat4 model;
    vec4 meshCenter;           //vec3 meshCenter, alpha
    vec4 defaultAttr;         //timeDelta, duration, bounceTimes, scaleRange
};
layout (std430, binding = 0) buffer InstanceBuffer {
    InstanceData instances[];
};
#else
uniform mat4 u_model;
uniform vec3 u_meshCenter;
uniform float u_timeDelta;
uniform float u_duration;
uniform float u_bounceTimes;
uniform float u_scaleRange;//0-1
#endif

const float PI = 3.1415926;

void main()
{
    #ifdef ENABLE_SSBO
    int batchID = int(floor(a_batch + 0.1));
    InstanceData instance = instances[batchID];
    mat4 model = instance.model;
    vec3 meshCenter = instance.meshCenter.xyz;
    float timeDelta = instance.defaultAttr.x;
    float duration = instance.defaultAttr.y;
    float bounceTimes = instance.defaultAttr.z;
    float scaleRange = instance.defaultAttr.w;

    v_batchID = batchID;
    #else
    mat4 model = u_model;
    vec3 meshCenter = u_meshCenter;
    float timeDelta = u_timeDelta;
    float duration = u_duration;
    float bounceTimes = u_bounceTimes;
    float scaleRange = u_scaleRange;

    v_batchID = 0;
    #endif

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
