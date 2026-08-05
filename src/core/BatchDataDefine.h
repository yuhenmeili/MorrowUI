//
// Created by 0060328 on 25-10-20.
//

#ifndef BATCHDATADEFINE_H
#define BATCHDATADEFINE_H
#include <memory>
#include <vector>
#include "Vector3.h"
#include "Vector4.h"
#include "renderer/device/ResourceHandle.h"

namespace morrow {
class VertexArray;
class SSBOData;
// SSBO is now a ResourceHandle alias (HwSSBO), defined in ResourceHandle.h
class Transform;
class MeshFilter;
class Material;
class Mesh;
class ShaderStorageBuffer;

using namespace Math;

struct RenderBatch {
    std::string shaderName;
    bool isSSBOShader = false;
    std::shared_ptr<VertexArray> vertexArray;
    std::vector<std::shared_ptr<Material>> materials;
    std::vector<std::shared_ptr<MeshFilter>> meshFilters;
    std::vector<std::shared_ptr<Transform>> transforms;
    std::shared_ptr<ShaderStorageBuffer> ssbo;
};

struct DefaultBatchData1Attr {
    Matrix4 model;
    Vector4 attr;
};

struct DefaultBatchData2Attr {
    Matrix4 model;
    Vector4 attr1;
    Vector4 attr2;
};

struct DefaultBatchData3Attr {
    Matrix4 model;
    Vector4 attr1;
    Vector4 attr2;
    Vector4 attr3;
};
}
#endif //BATCHDATADEFINE_H
