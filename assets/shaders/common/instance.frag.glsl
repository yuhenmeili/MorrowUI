// 片段阶段入口 chunk：共享 accessor。
// 宿主 shader 需在使用前声明 flat in int v_batchID。
#ifdef ENABLE_SSBO
int instanceBatchID() {
    return v_batchID;
}
#else
int instanceBatchID() {
    return 0;
}
#endif

#include "common/instance_accessors.glsl"
