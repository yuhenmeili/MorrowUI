// 顶点阶段入口 chunk：Global UBO + 实例模型矩阵 + 共享 accessor。
// 宿主 shader 需在使用前声明 a_batch 属性。
#include "common/global.glsl"
#include "common/instance.glsl"

#ifdef ENABLE_SSBO
int instanceBatchID() {
    return int(floor(a_batch + 0.1));
}
mat4 instanceModel() {
    return instances[instanceBatchID()].model;
}
#else
uniform mat4 u_model;
int instanceBatchID() {
    return 0;
}
mat4 instanceModel() {
    return u_model;
}
#endif

#include "common/instance_accessors.glsl"
