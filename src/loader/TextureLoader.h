//
// Created by lance on 2022/10/13.
//

#ifndef MORROW_TEXTURE_FROM_FILE_H
#define MORROW_TEXTURE_FROM_FILE_H

#include <cstddef>
#include <string>
#include <cstdint>

namespace morrow
{
class TextureFromFile
{
public:
    static unsigned char* load(char const* imageUri, int32_t* imageWidth, int32_t* imageHeight, int32_t *comp = nullptr, int32_t req_comp = 4);

    /// 从内存中的编码图像字节（png/jpg 等 stb 支持的格式）解码。
    static unsigned char* loadFromMemory(unsigned char const* imageData, size_t imageSize, int32_t* imageWidth, int32_t* imageHeight, int32_t *comp = nullptr, int32_t req_comp = 4);

    static float* loadFloat(char const* imageUri, int32_t* imageWidth, int32_t* imageHeight, int32_t *comp = nullptr, int32_t req_comp = 3);

    static void free(void* data);

    static int32_t save(char const* filename, int32_t x, int32_t y, int32_t comp, const void* data, int32_t stride_bytes,bool flip_vertically = false);

    static int32_t saveHdr(char const* filename, int32_t x, int32_t y, int32_t comp, const float* data, bool flip_vertically = false);
};

}


#endif //MORROW_TEXTURE_FROM_FILE_H
