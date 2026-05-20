layout (location = 0) in vec3 a_position;
layout (location = 1) in float a_batch;
layout (location = 2) in vec4 a_color;
layout (location = 3) in vec2 a_texCoord;

layout (location = 0) out vec3 v_position;
layout (location = 1) flat out int v_batchID;
layout (location = 2) out vec4 v_color;
layout (location = 3) out vec2 v_texCoord;

layout (std140) uniform Global {
    mat4 projectionView;
};

#ifdef ENABLE_SSBO
struct InstanceData {
    mat4 model;             //模型矩阵
    vec4 displaySize;       //width, height, 0, 0
    vec4 imageAttr;         //rounding, alpha, 0, 0
};
layout (std430, binding = 0) buffer InstanceBuffer {
    InstanceData instances[];
};
#else
uniform mat4 u_model;
#endif

void main() {
#ifdef ENABLE_SSBO
    // 使用a_batch获取当前实例索引
    int batchID = int(floor(a_batch + 0.1));
    InstanceData instance = instances[batchID];
    // 使用实例的模型矩阵变换顶点位置
    vec4 worldPosition = instance.model * vec4(a_position, 1.0);
    gl_Position = projectionView * worldPosition;
    v_batchID = batchID;
#else
    gl_Position = projectionView * u_model * vec4(a_position, 1.0);
    v_batchID = 0;
#endif

    // 传递属性到片段着色器
    v_position = a_position;
    v_color = a_color;
    v_texCoord = a_texCoord;
}
