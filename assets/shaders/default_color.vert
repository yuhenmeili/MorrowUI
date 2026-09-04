#pragma morrow ssbo
layout (location = 0) in vec3 a_position;
layout (location = 1) in float a_batch;
layout (location = 2) in vec4 a_color;

layout (location = 0) out vec3 v_position;
layout (location = 1) flat out int v_batchID;
layout (location = 2) out vec4 v_color;

#include "common/instance.vert.glsl"

void main() {
    gl_Position = projectionView * instanceModel() * vec4(a_position, 1.0);
    v_batchID = instanceBatchID();
    v_position = a_position;
    v_color = a_color;
}
