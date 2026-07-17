#include "Scene3DIBLLoader.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>

#include "Texture.h"
#include "Log.h"

namespace morrow {
namespace {
struct IBLMetadata {
    float rgbmRange = 64.0f;
    std::vector<int32_t> specularMipWidths = {512, 256, 128, 64, 32};
    std::vector<int32_t> specularMipHeights = {256, 128, 64, 32, 16};
    std::vector<int32_t> specularMipOffsetsY = {0, 256, 384, 448, 480};
};

TextureSharedPtr createSceneTexture(const std::string& path) {
    auto texture = Texture::create(ImageType::IMAGE);
    texture->setImageUrl(path);
    texture->setMinFilterType(SamplerMinFilter::LINEAR);
    texture->setMagFilterType(SamplerMagFilter::LINEAR);
    return texture;
}

std::string trim(const std::string& value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::vector<int32_t> parseIntList(const std::string& csv) {
    std::vector<int32_t> values;
    std::stringstream ss(csv);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item = trim(item);
        if (!item.empty()) {
            values.push_back(static_cast<int32_t>(std::stoi(item)));
        }
    }
    return values;
}

IBLMetadata loadMetadata(const std::string& iblDirectory) {
    IBLMetadata metadata;
    const std::string metaPath = iblDirectory + "/ibl_meta.txt";
    std::ifstream in(metaPath);
    if (!in.is_open()) {
        LOG_I("IBL metadata not found at {}, using default atlas layout", metaPath);
        return metadata;
    }

    std::string line;
    while (std::getline(in, line)) {
        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));
        if (key == "rgbmRange") {
            metadata.rgbmRange = std::stof(value);
        } else if (key == "specularMipWidths") {
            metadata.specularMipWidths = parseIntList(value);
        } else if (key == "specularMipHeights") {
            metadata.specularMipHeights = parseIntList(value);
        } else if (key == "specularMipOffsetsY") {
            metadata.specularMipOffsetsY = parseIntList(value);
        }
    }

    const size_t mipCount = std::min({metadata.specularMipWidths.size(), metadata.specularMipHeights.size(), metadata.specularMipOffsetsY.size()});
    metadata.specularMipWidths.resize(mipCount);
    metadata.specularMipHeights.resize(mipCount);
    metadata.specularMipOffsetsY.resize(mipCount);
    if (mipCount == 0) {
        LOG_I("IBL metadata at {} is incomplete, falling back to default atlas layout", metaPath);
        return {};
    }
    return metadata;
}
} // namespace

bool Scene3DIBLLoader::loadFromDirectory(const std::string& iblDirectory,
                                         Scene3DIBLState& outIBL,
                                         float intensity) {
    if (iblDirectory.empty()) {
        LOG_W("Scene3DIBLLoader: empty IBL directory, clearing IBL state");
        outIBL = {};
        return false;
    }

    const IBLMetadata metadata = loadMetadata(iblDirectory);
    outIBL.irradianceTexture = createSceneTexture(iblDirectory + "/irradiance_equirect.png");
    outIBL.specularTexture = createSceneTexture(iblDirectory + "/specular_equirect.png");
    outIBL.brdfLUTTexture = createSceneTexture(iblDirectory + "/brdf_lut.png");
    outIBL.rgbmRange = metadata.rgbmRange;
    outIBL.intensity = intensity;
    outIBL.specularMipWidths = metadata.specularMipWidths;
    outIBL.specularMipHeights = metadata.specularMipHeights;
    outIBL.specularMipOffsetsY = metadata.specularMipOffsetsY;

    LOG_I("Scene3D IBL loaded from {}", iblDirectory);
    return true;
}
} // namespace morrow
