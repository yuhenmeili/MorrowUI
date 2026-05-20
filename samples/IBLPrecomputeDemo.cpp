#include <iostream>
#include <string>

#include "IBLPrecompute.h"

using namespace morrow;

int main(int argc, char** argv) {
    const std::string defaultHDR = "../assets/textures/hdr/symmetrical_garden_1k.hdr";
    const std::string inputHDR = (argc > 1) ? argv[1] : defaultHDR;
    const std::string outputDirectory = (argc > 2) ? argv[2] : std::string();

    IBLPrecomputeOptions options;
    options.inputHDRPath = inputHDR;
    options.outputDirectory = outputDirectory;
    options.irradianceWidth = 128;
    options.irradianceHeight = 64;
    options.irradianceSampleCount = 256;
    options.specularBaseWidth = 512;
    options.specularBaseHeight = 256;
    options.specularMipCount = 5;
    options.specularSampleCount = 256;
    options.brdfLUTSize = 256;
    options.brdfSampleCount = 512;
    options.rgbmRange = 64.0f;
    options.writeHDRDebugImages = false;

    IBLPrecomputeResult result;
    std::string error;
    if (!IBLPrecompute::bake(options, result, error)) {
        std::cerr << "IBL precompute failed: " << error << std::endl;
        return 1;
    }

    std::cout << "IBL precompute succeeded." << std::endl;
    std::cout << "HDR input:      " << result.resolvedInputHDRPath << std::endl;
    std::cout << "Output folder:  " << result.outputDirectory << std::endl;
    std::cout << "Irradiance:     " << result.irradiancePNGPath << std::endl;
    std::cout << "Specular atlas: " << result.specularPNGPath << std::endl;
    std::cout << "BRDF LUT:       " << result.brdfLUTPNGPath << std::endl;
    std::cout << "Specular atlas size: " << result.specularAtlasWidth << " x " << result.specularAtlasHeight << std::endl;

    return 0;
}

