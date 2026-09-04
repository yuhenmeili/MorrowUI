// 统一 UI 实例布局（SHADER_SOURCE_ORGANIZATION_PROPOSAL.md §4）
// 全部 UI 组件共用；CPU 侧每帧全量填充，未使用的槽位写默认值：
// color0/color1 = (1,1,1,1)，geomAttr = (0,0,0,1)，stateAttr/extraAttr = (0,0,0,0)
#ifdef ENABLE_SSBO
struct InstanceData {
    mat4 model;
    vec4 color0;     //主色：bgColor / fontColor / defaultColor / trackColor ...
    vec4 color1;     //副色或辅助向量：fillColor / meshCenter.xyz + alpha
    vec4 geomAttr;   //xy = displaySize, z = rounding, w = alpha
    vec4 stateAttr;  //状态/开关：useTexture / progress / direction ...
    vec4 extraAttr;  //自定义参数：动画参数等
};
layout (std430, binding = 0) buffer InstanceBuffer {
    InstanceData instances[];
};
#endif
