//
// Created by 0060328 on 25-10-23.
//

#ifndef GLOBALTOOLS_H
#define GLOBALTOOLS_H

#include <string>

#if defined(QNX) && defined(__has_include)
#if __has_include(<pmem.h>)
#include <pmem.h>
#define MORROW_HAS_PMEM 1
#else
#define MORROW_HAS_PMEM 0
#endif
#elif defined(QNX)
#include <pmem.h>
#define MORROW_HAS_PMEM 1
#else
#define MORROW_HAS_PMEM 0
#endif

namespace morrow {
namespace global_tools {
// 定义一个辅助函数来计算偏移量
template <typename T, typename U>
constexpr size_t offsetOf(U T::* member) {
    return reinterpret_cast<size_t>(&((static_cast<T*>(nullptr))->*member));
}

constexpr float kPi = 3.14159265358979323846f;

#if MORROW_HAS_PMEM

bool loadPmemData(void* pmemHdl, std::string fileName)
{
    int flags = (PMEM_FLAGS_PHYS_CONTIG | PMEM_FLAGS_CACHE_NONE | PMEM_FLAGS_SHMEM);
    // 3 for rgb, 4 for camera nums
    int stride = (TEST_BUFFER_WIDTH + (256 - 1)) & ~(256 - 1);
    int size = stride * TEST_BUFFER_HEIGHT * 3;

    FILE* fp = fopen(fileName, "r");
    if (!fp) {
        LOG_E("open file error");
        return false;
    }

    void* pmm_hd = NULL;
    pmemHdl = pmem_malloc_ext_v2(size, PMEM_GRAPHICS_FRAMEBUFFER_ID,
                                        flags, PMEM_ALIGNMENT_4K, 0,
                                        (pmem_handle_t*) &pmm_hd, NULL);

    if (NULL == pmm_hd) {
        LOG_E("pmm_hd is Null,Pmem_malloc  error!\n");
        return false;
    }

    fseek(fp, 0, SEEK_SET);
    for (auto i = 0; i < TEST_BUFFER_HEIGHT; ++i) {
        fread((char*) ((unsigned char*) pmemHdl + i * stride * 3),
              TEST_BUFFER_WIDTH * 3, 1, fp);
    }

    fclose(fp);

    return true;
}

#elif defined(QNX)

inline bool loadPmemData(void* pmemHdl, std::string fileName)
{
    (void)pmemHdl;
    (void)fileName;
    return false;
}

#endif

}
}
#endif //GLOBALTOOLS_H
