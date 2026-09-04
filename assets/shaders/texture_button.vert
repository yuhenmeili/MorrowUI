#pragma morrow ssbo
layout (location = 0) in vec3 a_position;
layout (location = 1) in float a_batch;
layout (location = 2) in vec2 a_texCoord;

layout (location = 0) flat out int v_batchID;
layout (location = 1) out vec2 v_texCoord;

#include "common/instance.vert.glsl"

void main() {
    gl_Position = projectionView * instanceModel() * vec4(a_position, 1.0);
    v_batchID = instanceBatchID();
    v_texCoord = a_texCoord;
}
