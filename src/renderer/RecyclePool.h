//
// 泛化回收池：主线程 acquire，渲染线程在 GPU Fence 完成后 release 回池。
// 支持 VBOData、UBOData、SSBOData 等。
//

#ifndef MORROW_RENDERER_RECYCLEPOOL_H
#define MORROW_RENDERER_RECYCLEPOOL_H

#include <memory>
#include <queue>
#include <mutex>
#include <vector>

namespace morrow {

template<typename T>
class RecyclePool {
public:
    using Ptr = std::shared_ptr<T>;

    /// 主线程：从池中取出一块（无则新建）
    Ptr acquire() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_pool.empty())
            return std::make_shared<T>();
        Ptr p = std::move(m_pool.front());
        m_pool.pop();
        return p;
    }

    /// 渲染线程：GPU 已完成使用后，将多块归还池中
    void release(std::vector<Ptr>&& list) {
        if (list.empty()) return;
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& p : list)
            m_pool.push(std::move(p));
    }

private:
    std::queue<Ptr> m_pool;
    std::mutex m_mutex;
};

} // morrow

#endif // MORROW_RENDERER_RECYCLEPOOL_H
