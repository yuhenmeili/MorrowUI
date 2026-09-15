//
// Created by lance on 2022/10/13.
//

#include "TextureLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace morrow
{
unsigned char* TextureFromFile::load(char const* imageUri, int32_t* imageWidth, int32_t* imageHeight, int32_t* comp, int32_t req_comp)
{
    // stbi_set_flip_vertically_on_load(true);
    return stbi_load(imageUri, imageWidth, imageHeight, comp, req_comp);
}

float* TextureFromFile::loadFloat(char const* imageUri, int32_t* imageWidth, int32_t* imageHeight, int32_t* comp, int32_t req_comp)
{
    return stbi_loadf(imageUri, imageWidth, imageHeight, comp, req_comp);
}

void TextureFromFile::free(void* data)
{
    stbi_image_free(data);
}

int32_t TextureFromFile::save(const char* filename, int32_t x, int32_t y, int32_t comp, const void* data, int32_t stride_bytes,
                          bool flip_vertically)
{
    stbi_flip_vertically_on_write(flip_vertically ? 1 : 0);
    return stbi_write_png(filename, x, y, comp, data, stride_bytes);
}

int32_t TextureFromFile::saveHdr(const char* filename, int32_t x, int32_t y, int32_t comp, const float* data,
                                 bool flip_vertically)
{
    stbi_flip_vertically_on_write(flip_vertically ? 1 : 0);
    return stbi_write_hdr(filename, x, y, comp, data);
}
}