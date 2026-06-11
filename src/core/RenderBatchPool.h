//
// Created by 0060328 on 26-6-8.
//

#ifndef RENDERBATCHPOOL_H
#define RENDERBATCHPOOL_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "BatchDataDefine.h"
#include "utils/Singleton.h"

namespace morrow {

/// 以 shaderName + isSSBOShader 为复合键的 RenderBatch 对象池
/// 复用 RenderBatch 避免帧间无限增长导致内存泄漏
class RenderBatchPool : public Singleton<RenderBatchPool> {
    friend class Singleton<RenderBatchPool>;

public:
    /// 从池中获取一个 RenderBatch（池中有则复用，无则新建）
    RenderBatch acquire(const std::string& shaderName, bool isSSBOShader);

    /// 将单个 RenderBatch 归还池中
    void release(RenderBatch&& batch);

    /// 批量归还（帧结束时调用），清空传入的 vector
    void releaseAll(std::vector<RenderBatch>& batches);

    /// 清空整个对象池
    void clear();

private:
    RenderBatchPool() = default;
    ~RenderBatchPool() = default;

    /// 生成复合键："shaderName|0" 或 "shaderName|1"
    static std::string makeKey(const std::string& shaderName, bool isSSBOShader);

    std::unordered_map<std::string, std::vector<RenderBatch>> m_pool;
};

} // namespace morrow

#endif // RENDERBATCHPOOL_H
