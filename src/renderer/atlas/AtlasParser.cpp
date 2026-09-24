//
// Created by lance on 24-7-1.
//

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>
#include "AtlasParser.h"

#include "ToolUtils.h"
#include "utils/Log.h"

namespace morrow {
namespace {
// 去除换行与空格，行内缩进不影响解析。
std::string trim(const std::string& str) {
    std::string result = str;
    ToolUtils::replaceAll(result, "\n", "");
    ToolUtils::replaceAll(result, "\r", "");
    ToolUtils::replaceAll(result, " ", "");
    return result;
}

// 以第一个冒号分割 key 与 value，value 内再按逗号分割。
std::vector<std::string> splitEntry(const std::string& line) {
    std::vector<std::string> entry;
    auto colon = line.find(':');
    if (colon == std::string::npos) {
        entry.emplace_back(line);
        return entry;
    }
    entry.emplace_back(trim(line.substr(0, colon)));
    size_t lastMatch = colon + 1;
    while (true) {
        auto comma = line.find(',', lastMatch);
        if (comma == std::string::npos) {
            entry.emplace_back(trim(line.substr(lastMatch)));
            return entry;
        }
        entry.emplace_back(trim(line.substr(lastMatch, comma - lastMatch)));
        lastMatch = comma + 1;
    }
}
}

std::vector<int32_t> AtlasFrame::findValue(const std::string& name) const {
    for (size_t i = 0; i < names.size(); ++i) {
        if (name == names[i]) {
            return values[i];
        }
    }
    return {};
}

void AtlasParser::parse(const std::string& atlasFilePath, bool flip) {
    std::ifstream file(atlasFilePath, std::ios::binary);
    if (!file.is_open()) {
        LOG_I("Failed to open file {}.", atlasFilePath);
        return;
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::istringstream stream(content);
    parseStream(stream, flip, atlasFilePath);
}

void AtlasParser::parseBuffer(const unsigned char* atlasData, size_t size, bool flip) {
    if (!atlasData || size == 0) {
        LOG_I("Failed to parse atlas buffer: empty data");
        return;
    }
    std::string content(reinterpret_cast<const char*>(atlasData), size);
    std::istringstream stream(content);
    parseStream(stream, flip, "buffer");
}

void AtlasParser::parseStream(std::istream& stream, bool flip, const std::string& sourceName) {
    bool hasIndexes = false;
    AtlasPageSharedPtr page;
    AtlasFrameSharedPtr frame;
    std::string line;
    while (std::getline(stream, line)) {
        line = trim(line);
        if (line.empty()) {
            continue;
        }
        auto entry = splitEntry(line);
        const std::string& key = entry[0];
        if (ToolUtils::startsWith(key, "atlas_")) {
            // 图集纹理文件名行，开启新页。
            page = std::make_shared<AtlasPage>();
            page->name = key;
            m_pages.emplace_back(page);
        } else if (key == "size") {
            if (page) {
                page->width = std::stoi(entry[1]);
                page->height = std::stoi(entry[2]);
            } else {
                LOG_I("Failed to load atlas file {}. size before page", sourceName);
            }
        } else if (key == "bounds") {
            if (frame) {
                frame->left = std::stoi(entry[1]);
                frame->top = std::stoi(entry[2]);
                frame->width = std::stoi(entry[3]);
                frame->height = std::stoi(entry[4]);
            } else {
                LOG_I("Failed to load atlas file {}. bounds before frame", sourceName);
            }
        } else if (key == "index") {
            if (frame) {
                frame->index = std::stoi(entry[1]);
                if (frame->index != -1) {
                    hasIndexes = true;
                }
            } else {
                LOG_I("Failed to load atlas file {}. index before frame", sourceName);
            }
        } else if (entry.size() == 1) {
            // 其余无冒号行为帧名称，开启新帧。
            frame = std::make_shared<AtlasFrame>();
            frame->name = key;
            frame->page = page;
            frame->flip = flip;
            m_frames.emplace_back(frame);
        }
        // 其余带冒号的头部字段（repeat/format/filter/pma 等）当前引擎不使用，忽略。
    }

    // 索引模式：帧按 index 升序排列。
    if (hasIndexes) {
        std::stable_sort(m_frames.begin(), m_frames.end(), [](const AtlasFrameSharedPtr& a, const AtlasFrameSharedPtr& b) {
            return a->index < b->index;
        });
    }
}
} // MORROWGUI