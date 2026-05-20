//
// Created by 0060328 on 25-10-9.
//

#ifndef FONTTEXTURE_H
#define FONTTEXTURE_H
#include <vector>

#include "Texture.h"

namespace morrow {
class FontTexture : public Texture{
public:
    FontTexture(int32_t width, int32_t height);

    bool Initialize();

    void UpdateRegion(int32_t x, int32_t y, int32_t width, int32_t height, const unsigned char* data);

    /// 将所有 UpdateRegion 写入的 CPU 数据一次性提交到 GPU，应在批量 UpdateRegion 完成后调用
    void FlushToGPU();

private:
    std::vector<unsigned char> m_fontData;
    bool m_dirty = false;
};
} // morrow

#endif //FONTTEXTURE_H
