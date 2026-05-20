//
// Created by lance on 2023/11/22.
//

#ifndef MORROW_CORE_TOOLUTILS_H_
#define MORROW_CORE_TOOLUTILS_H_

#include <cstdint>
#include <vector>

#include "DriverEnums.h"
#include "GlobalDefine.h"

namespace morrow
{
namespace ToolUtils
{
std::wstring s2ws(const std::string& str);

int32_t countWords(const std::string& text, std::vector<std::string> &words);

bool startsWith(const std::string& s, std::string& sub);

bool startsWith(const std::string& s, const char* sub);

bool endsWith(const std::string& s, std::string& sub);

bool endsWith(const std::string& s, const char* sub);

int32_t bitsLength(uint32_t n);

std::string replaceAll(std::string &str, const std::string& oldStr, const std::string& newStr);

void debugTexture(void* dataPtr, int32_t imageWidth, int32_t imageHeight, const std::string& url, ImageType type, PixelDataFormat format);
}
} // MORROWGUI

#endif //MORROW_CORE_TOOLUTILS_H_
