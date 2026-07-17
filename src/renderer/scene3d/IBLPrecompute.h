#ifndef MORROW_IBL_PRECOMPUTE_H
#define MORROW_IBL_PRECOMPUTE_H

#include <string>
#include <vector>

namespace morrow {

struct IBLPrecomputeOptions {
    std::string inputHDRPath;
    std::string outputDirectory;

    int irradianceWidth = 128;
    int irradianceHeight = 64;
    int irradianceSampleCount = 256;

    int specularBaseWidth = 512;
    int specularBaseHeight = 256;
    int specularMipCount = 5;
    int specularSampleCount = 256;

    int brdfLUTSize = 256;
    int brdfSampleCount = 512;

    float rgbmRange = 64.0f;
    bool writeHDRDebugImages = false;
};

struct IBLPrecomputeResult {
    std::string resolvedInputHDRPath;
    std::string outputDirectory;

    std::string irradiancePNGPath;
    std::string specularPNGPath;
    std::string brdfLUTPNGPath;

    std::string irradianceHDRPath;
    std::string specularHDRPath;

    int specularAtlasWidth = 0;
    int specularAtlasHeight = 0;
    std::vector<int> specularMipWidths;
    std::vector<int> specularMipHeights;
    std::vector<int> specularMipOffsetsY;
};

class IBLPrecompute {
public:
    static bool bake(const IBLPrecomputeOptions& options, IBLPrecomputeResult& outResult, std::string& outError);
};

} // namespace morrow

#endif // MORROW_IBL_PRECOMPUTE_H

