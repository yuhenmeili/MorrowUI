//
// Created by lance on 24-7-1.
//

#include <functional>
#include <algorithm>
#include <unordered_map>
#include "TextureAtlasData.h"

#include <fstream>

#include "../../utils/Log.h"
#include "ToolUtils.h"

namespace morrow
{
std::vector<int32_t> Region::findValue(std::string& name)
{
    std::vector<int32_t> result;
    if (!this->names.empty()) {
        for (int32_t indexName = 0; indexName < this->names.size(); ++indexName) {
            if (name == this->names[indexName]) {
                return this->values[indexName];
            }
        }
    }
    return result;
}

TextureAtlasData::~TextureAtlasData()
{
    this->pages.clear();
    this->regions.clear();
}

void TextureAtlasData::load(const std::string& packFileUrl, bool flip)
{
    auto entry = std::vector<std::string>(5);
    std::unordered_map<std::string, std::function<void(const PageSharedPtr&)>> pageFields;
    // Populate the map with a lambda function
    pageFields["size"] = [&](const PageSharedPtr& page) {
        page->width = std::stoi(entry[1]); // Assuming entry is accessible
        page->height = std::stoi(entry[2]); // Ensure error handling for stoi
    };
//        pageFields["format"] = [&](Page& page) {
//            page.format = Format.valueOf(entry[1]);
//        };
//        pageFields["filter"] = [&](Page& page) {
//            page.minFilter = TextureFilter.valueOf(entry[1]);
//            page.magFilter = TextureFilter.valueOf(entry[2]);
//            page.useMipMaps = page.minFilter.isMipMap();
//        };
    pageFields["repeat"] = [&](const PageSharedPtr& page) {
//            if (entry[1].indexOf(120) != -1) {
//                page.uWrap = TextureWrap.Repeat;
//            }
//
//            if (entry[1].indexOf(121) != -1) {
//                page.vWrap = TextureWrap.Repeat;
//            }
    };
//        pageFields["pma"] = [&](Page& page) {
//            page.pma = entry[1].equals("true");
//        };

    std::unordered_map<std::string, std::function<void(const RegionSharedPtr&)>> regionFields;
    std::array<bool, 1> hasIndexes = {false};
//        regionFields["xy"] = [&](RegionSharedPtr region) {
//            region->left = std::stoi(entry[1]);
//            region->top = std::stoi(entry[2]);
//        };
//        regionFields["size"] = [&](RegionSharedPtr region) {
//            region->width = std::stoi(entry[1]);
//            region->height = std::stoi(entry[2]);
//        };
    regionFields["bounds"] = [&](const RegionSharedPtr& region) {
        region->left = std::stoi(entry[1]);
        region->top = std::stoi(entry[2]);
        region->width = std::stoi(entry[3]);
        region->height = std::stoi(entry[4]);
    };
//        regionFields["offset"] = [&](RegionSharedPtr region) {
//            region->offsetX = std::stof(entry[1]);
//            region->offsetY = std::stof(entry[2]);
//        };
//        regionFields["orig"] = [&](RegionSharedPtr region) {
//            region->originalWidth = std::stoi(entry[1]);
//            region->originalHeight = std::stoi(entry[2]);
//        };
//        regionFields["offsets"] = [&](RegionSharedPtr region) {
//            region->offsetX = std::stof(entry[1]);
//            region->offsetY = std::stof(entry[2]);
//            region->originalWidth = std::stoi(entry[3]);
//            region->originalHeight = std::stoi(entry[4]);
//        };
//        regionFields["rotate"] = [&](RegionSharedPtr region) {
//            std::string value = entry[1];
//            if (value == "true") {
//                region->degrees = 90;
//            } else if (value != "false") {
//                region->degrees = std::stoi(value);
//            }
//            region->rotate = region->degrees == 90;
//        };
    regionFields["index"] = [&](const RegionSharedPtr& region) {
        region->index = std::stoi(entry[1]);
        if (region->index != -1) {
            hasIndexes[0] = true;
        }
    };
    std::ifstream file(packFileUrl);
    if (!file.is_open()) {
        LOG_I("Failed to open file {}.", packFileUrl);
        return;
    }
    char* result = nullptr;
    size_t len = 0;
    std::string line;
    PageSharedPtr page;
    RegionSharedPtr region;
    while (std::getline(file, line)) {
        if (line.empty() || (line = trimString(line)).empty()) {
            continue;
        }
        auto index = readEntry(entry, line);
        if (ToolUtils::startsWith(entry[0], "atlas_")) {
            page = std::make_shared<Page>();
            page->name = entry[0];
            this->pages.emplace_back(page);
        } else if (pageFields.find(entry[0]) != pageFields.end()) {
            if (page) {
                pageFields[entry[0]](page);
            } else {
                LOG_I("Failed to load atlas file {}. prefix error", packFileUrl.c_str());
            }
        } else if (regionFields.find(entry[0]) != regionFields.end()) {
            if (region) {
                regionFields[entry[0]](region);
            } else {
                LOG_I("Failed to load atlas file {}. format error", packFileUrl.c_str());
            }
        } else if(1 == index){
            region = std::make_shared<Region>();
            region->name = entry[0];
            region->page = page;
            this->regions.emplace_back(region);
        }
    }

    if (hasIndexes[0]) {
        std::stable_sort(this->regions.begin(), this->regions.end(), [](const RegionSharedPtr& a, const RegionSharedPtr& b) {
            return a->index < b->index;
        });
    }
}

std::vector<PageSharedPtr>& TextureAtlasData::getPages()
{
    return this->pages;
}

std::vector<RegionSharedPtr>& TextureAtlasData::getRegions()
{
    return this->regions;
}

std::basic_string<char> TextureAtlasData::trimString(std::basic_string<char> str)
{
    if (str.empty()) {
        return str;
    }
    ToolUtils::replaceAll(str, "\n", "");
    ToolUtils::replaceAll(str, "\r", "");
    ToolUtils::replaceAll(str, " ", "");
    return str;
}

int32_t TextureAtlasData::readEntry(std::vector<std::string>& entry, std::string& line)
{
    int32_t colon = line.find(':');
    entry[0] = trimString(line.substr(0, colon));
    if (colon == std::string::npos) {
        return 1;
    }
    int32_t index = 1;
    int32_t lastMatch = colon + 1;
    while (true) {
        int32_t comma = line.find(',', lastMatch);
        if (comma == std::string::npos) {
            entry[index] = trimString(line.substr(lastMatch));
            return index;
        }
        entry[index] = trimString(line.substr(lastMatch, comma - lastMatch));
        lastMatch = comma + 1;
        if (index == 4) {
            return 4;
        }
        ++index;
    }
}
} // MORROWGUI