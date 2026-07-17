//
// Created by 0060328 on 25-10-23.
//

#ifndef SSBOMANAGER_H
#define SSBOMANAGER_H
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "ShaderStorageBuffer.h"

namespace morrow {

class SSBOManager {
public:
    // 为指定Shader更新SSBO数据
    void updateSSBOForShader(const std::string& shaderName, RenderBatch& batch);

    bool hasLayout(const std::string& shaderName) const;

private:
    // 注册不同Shader的SSBO布局
    void registerSSBOLayout(SSBOLayout& layout);

    std::unordered_map<std::string, SSBOLayout> m_shaderLayouts = {};
    std::unordered_map<std::string, std::vector<uint8_t>> m_ssboBuffers = {};
};
} // morrow

#endif //SSBOMANAGER_H
