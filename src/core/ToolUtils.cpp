//
// Created by lance on 2023/11/22.
//

#include <fstream>
#include <codecvt>
#include <locale>
#include <cstring>
#include "ToolUtils.h"
#include "TextureLoader.h"
#include "PixelFormat.h"
#include <iostream>
#include <sstream>

namespace morrow
{
namespace ToolUtils
{
std::wstring s2ws(const std::string& str)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.from_bytes(str);
}

int32_t countWords(const std::string& text, std::vector<std::string>& words)
{
    std::istringstream stream(text);
    std::string word;
    int32_t count = 0;
    while (stream >> word) {
        ++count;
        words.emplace_back(word);
    }
    return count;
}

bool startsWith(const std::string& s, std::string& sub)
{
    if (s.length() < sub.length())
        return false;
    return s.find(sub) == 0;
}

bool startsWith(const std::string& s, const char* sub)
{
    if (s.length() < strlen(sub))
        return false;
    return s.find(sub) == 0;
}

bool endsWith(const std::string& s, std::string& sub)
{
    if (s.length() < sub.length())
        return false;
    return s.rfind(sub) == (s.length() - sub.length());
}

bool endsWith(const std::string& s, const char* sub)
{
    if (s.length() < strlen(sub))
        return false;
    return s.rfind(sub) == (s.length() - strlen(sub));
}

int32_t bitsLength(uint32_t n)
{
    return n ? bitsLength(n / 2) + 1 : 0;
}

std::string replaceAll(std::string& str, const std::string& oldStr, const std::string& newStr)
{
    std::string::size_type pos = str.find(oldStr);
    while (pos != std::string::npos) {
        str.replace(pos, oldStr.size(), newStr);
        pos = str.find(oldStr);
    }
    return str;
}

void debugTexture(void* dataPtr, int32_t imageWidth, int32_t imageHeight, const std::string& url, ImageType type, PixelDataFormat format)
{
    if (type == ImageType::OES) {
        std::FILE* fp = nullptr;
        std::string suffix = format == PixelDataFormat::RGB ? ".rgb" : ".rgba";
        fp = fopen((url + suffix).c_str(), "wb+");
        if (fp) {
            uint32_t alignment = 64;
            uint32_t numbers = 4;
            if (format == PixelDataFormat::RGB) {
                alignment = 256;
                numbers = 3;
            }
            uint32_t stride = numbers * ((imageWidth + (alignment - 1)) & ~(alignment - 1));
            uint32_t len = (((stride * imageHeight) + 4096 - 1) & ~(4096 - 1));
            fwrite(dataPtr, 1, len, fp);
            fflush(fp);
            fclose(fp);
        }
    } else {
        //只考虑jpg和png
        std::string suffix = format == PixelDataFormat::RGB ? ".jpg" : ".png";
        TextureFromFile::save((url + suffix).c_str(),
                              imageWidth,
                              imageHeight,
                              PixelFormat::componentsLength(format),
                              dataPtr,
                              0);
    }
}
}
} // MORROWGUI