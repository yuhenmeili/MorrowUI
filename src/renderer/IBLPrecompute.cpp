#include "IBLPrecompute.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <utility>

#include "TextureLoader.h"
#include "MathUtils.h"
#include "Vector3.h"

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace morrow {
using namespace Math;

namespace {
constexpr float kTwoPi = MR_PI * 2.0f;

struct FloatImage {
    int width = 0;
    int height = 0;
    std::vector<Vector3> pixels;

    bool isValid() const {
        return width > 0 && height > 0 && static_cast<int>(pixels.size()) == width * height;
    }

    Vector3 get(int x, int y) const {
        return pixels[y * width + x];
    }

    void set(int x, int y, const Vector3& value) {
        pixels[y * width + x] = value;
    }
};

struct RGBA8Image {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> pixels;

    RGBA8Image() = default;

    RGBA8Image(int w, int h)
        : width(w)
        , height(h)
        , pixels(static_cast<size_t>(w * h * 4), 0) {
    }

    void set(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        const size_t index = static_cast<size_t>((y * width + x) * 4);
        pixels[index + 0] = r;
        pixels[index + 1] = g;
        pixels[index + 2] = b;
        pixels[index + 3] = a;
    }
};

static float saturate(float value) {
    return std::max(0.0f, std::min(1.0f, value));
}

static float positiveModulo(float value, float mod) {
    float result = std::fmod(value, mod);
    if (result < 0.0f) {
        result += mod;
    }
    return result;
}

static int wrapInt(int value, int mod) {
    if (mod <= 0) {
        return 0;
    }
    int result = value % mod;
    if (result < 0) {
        result += mod;
    }
    return result;
}

static std::string normalizePath(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

static std::string joinPath(const std::string& left, const std::string& right) {
    if (left.empty()) {
        return right;
    }
    if (right.empty()) {
        return left;
    }
    if (left.back() == '/' || left.back() == '\\') {
        return left + right;
    }
    return left + "/" + right;
}

static std::string parentDirectory(const std::string& path) {
    const std::string normalized = normalizePath(path);
    const size_t pos = normalized.find_last_of('/');
    if (pos == std::string::npos) {
        return {};
    }
    return normalized.substr(0, pos);
}

static std::string fileStem(const std::string& path) {
    const std::string normalized = normalizePath(path);
    const size_t slash = normalized.find_last_of('/');
    const size_t start = (slash == std::string::npos) ? 0 : (slash + 1);
    const size_t dot = normalized.find_last_of('.');
    if (dot == std::string::npos || dot < start) {
        return normalized.substr(start);
    }
    return normalized.substr(start, dot - start);
}

static bool createDirectorySingle(const std::string& path) {
    if (path.empty()) {
        return true;
    }
#if defined(_WIN32)
    const int rc = _mkdir(path.c_str());
#else
    const int rc = mkdir(path.c_str(), 0755);
#endif
    return rc == 0 || errno == EEXIST;
}

static bool ensureDirectory(const std::string& directory) {
    const std::string normalized = normalizePath(directory);
    if (normalized.empty()) {
        return true;
    }

    std::string current;
    size_t start = 0;
    if (normalized.size() > 1 && normalized[1] == ':') {
        current = normalized.substr(0, 2);
        start = 2;
    } else if (!normalized.empty() && normalized[0] == '/') {
        current = "/";
        start = 1;
    }

    while (start < normalized.size()) {
        const size_t next = normalized.find('/', start);
        const std::string part = normalized.substr(start, next == std::string::npos ? std::string::npos : next - start);
        if (!part.empty()) {
            if (!current.empty() && current.back() != '/') {
                current += '/';
            }
            current += part;
            if (!createDirectorySingle(current)) {
                return false;
            }
        }
        if (next == std::string::npos) {
            break;
        }
        start = next + 1;
    }
    return true;
}

static bool ensureParentDirectory(const std::string& path) {
    return ensureDirectory(parentDirectory(path));
}

static Vector3 directionFromEquirect(float u, float v) {
    const float phi = (u - 0.5f) * kTwoPi;
    const float theta = saturate(v) * MR_PI;
    const float sinTheta = std::sin(theta);
    return Vector3(std::cos(phi) * sinTheta,
                   std::cos(theta),
                   std::sin(phi) * sinTheta).normalize();
}

static void directionToEquirect(const Vector3& dir, float& u, float& v) {
    Vector3 n = dir;
    n.normalize();
    const float phi = std::atan2(n.z, n.x);
    const float theta = std::acos(std::max(-1.0f, std::min(1.0f, n.y)));
    u = positiveModulo(phi / kTwoPi + 0.5f, 1.0f);
    v = saturate(theta / MR_PI);
}

static Vector3 sampleBilinear(const FloatImage& image, float u, float v) {
    if (!image.isValid()) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }

    u = positiveModulo(u, 1.0f);
    v = saturate(v);

    const float fx = u * image.width - 0.5f;
    const float fy = v * image.height - 0.5f;
    const int x0 = static_cast<int>(std::floor(fx));
    const int y0 = static_cast<int>(std::floor(fy));
    const int x1 = x0 + 1;
    const int y1 = std::min(y0 + 1, image.height - 1);
    const float tx = fx - std::floor(fx);
    const float ty = fy - std::floor(fy);

    const Vector3 c00 = image.get(wrapInt(x0, image.width), std::max(0, std::min(y0, image.height - 1)));
    const Vector3 c10 = image.get(wrapInt(x1, image.width), std::max(0, std::min(y0, image.height - 1)));
    const Vector3 c01 = image.get(wrapInt(x0, image.width), std::max(0, std::min(y1, image.height - 1)));
    const Vector3 c11 = image.get(wrapInt(x1, image.width), std::max(0, std::min(y1, image.height - 1)));

    const Vector3 c0 = c00 * (1.0f - tx) + c10 * tx;
    const Vector3 c1 = c01 * (1.0f - tx) + c11 * tx;
    return c0 * (1.0f - ty) + c1 * ty;
}

static Vector3 sampleEquirect(const FloatImage& image, const Vector3& dir) {
    float u = 0.0f;
    float v = 0.0f;
    directionToEquirect(dir, u, v);
    return sampleBilinear(image, u, v);
}

static float radicalInverseVdC(uint32_t bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return static_cast<float>(bits) * 2.3283064365386963e-10f;
}

static std::pair<float, float> hammersley(uint32_t i, uint32_t count) {
    return {static_cast<float>(i) / static_cast<float>(count), radicalInverseVdC(i)};
}

static void buildTangentBasis(const Vector3& normal, Vector3& tangent, Vector3& bitangent) {
    const Vector3 up = (std::fabs(normal.y) < 0.999f) ? Vector3(0.0f, 1.0f, 0.0f) : Vector3(1.0f, 0.0f, 0.0f);
    tangent = cross(up, normal).normalize();
    bitangent = cross(normal, tangent).normalize();
}

static Vector3 tangentToWorld(const Vector3& sample, const Vector3& normal) {
    Vector3 tangent;
    Vector3 bitangent;
    buildTangentBasis(normal, tangent, bitangent);
    return (tangent * sample.x + bitangent * sample.y + normal * sample.z).normalize();
}

static Vector3 cosineSampleHemisphere(float xi1, float xi2) {
    const float r = std::sqrt(xi1);
    const float phi = kTwoPi * xi2;
    const float x = r * std::cos(phi);
    const float y = r * std::sin(phi);
    const float z = std::sqrt(std::max(0.0f, 1.0f - x * x - y * y));
    return Vector3(x, y, z);
}

static Vector3 importanceSampleGGX(float xi1, float xi2, float roughness, const Vector3& normal) {
    const float a = roughness * roughness;
    const float a2 = a * a;

    const float phi = kTwoPi * xi1;
    const float cosTheta = std::sqrt((1.0f - xi2) / (1.0f + (a2 - 1.0f) * xi2));
    const float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));

    const Vector3 h(std::cos(phi) * sinTheta,
                    std::sin(phi) * sinTheta,
                    cosTheta);
    return tangentToWorld(h, normal);
}

static float geometrySchlickGGXIBL(float ndotv, float roughness) {
    const float a = roughness;
    const float k = (a * a) * 0.5f;
    return ndotv / std::max(ndotv * (1.0f - k) + k, 1e-5f);
}

static float geometrySmithIBL(float ndotv, float ndotl, float roughness) {
    return geometrySchlickGGXIBL(ndotv, roughness) * geometrySchlickGGXIBL(ndotl, roughness);
}

static Vector3 integrateIrradiance(const FloatImage& environment, const Vector3& normal, int sampleCount) {
    Vector3 sum(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < sampleCount; ++i) {
        const auto xi = hammersley(static_cast<uint32_t>(i), static_cast<uint32_t>(sampleCount));
        const Vector3 sampleDir = tangentToWorld(cosineSampleHemisphere(xi.first, xi.second), normal);
        sum += sampleEquirect(environment, sampleDir);
    }
    return sum * (MR_PI / static_cast<float>(sampleCount));
}

static Vector3 prefilterSpecular(const FloatImage& environment, const Vector3& reflectionDir, float roughness, int sampleCount) {
    const Vector3 normal = reflectionDir;
    const Vector3 view = reflectionDir;
    Vector3 prefiltered(0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;

    if (roughness <= 1e-4f) {
        return sampleEquirect(environment, reflectionDir);
    }

    for (int i = 0; i < sampleCount; ++i) {
        const auto xi = hammersley(static_cast<uint32_t>(i), static_cast<uint32_t>(sampleCount));
        const Vector3 h = importanceSampleGGX(xi.first, xi.second, roughness, normal);
        const float vdotH = std::max(dot(view, h), 0.0f);
        const Vector3 light = (h * (2.0f * vdotH) - view).normalize();
        const float ndotl = std::max(dot(normal, light), 0.0f);
        if (ndotl > 0.0f) {
            prefiltered += sampleEquirect(environment, light) * ndotl;
            totalWeight += ndotl;
        }
    }

    if (totalWeight <= 1e-5f) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }
    return prefiltered / totalWeight;
}

static Vector3 integrateBRDF(float ndotv, float roughness, int sampleCount) {
    Vector3 view(std::sqrt(std::max(0.0f, 1.0f - ndotv * ndotv)), 0.0f, ndotv);

    float a = 0.0f;
    float b = 0.0f;
    const Vector3 normal(0.0f, 0.0f, 1.0f);

    for (int i = 0; i < sampleCount; ++i) {
        const auto xi = hammersley(static_cast<uint32_t>(i), static_cast<uint32_t>(sampleCount));
        const Vector3 h = importanceSampleGGX(xi.first, xi.second, roughness, normal);
        const Vector3 light = (h * (2.0f * dot(view, h)) - view).normalize();

        const float ndotl = std::max(light.z, 0.0f);
        const float ndoth = std::max(h.z, 0.0f);
        const float vdoth = std::max(dot(view, h), 0.0f);

        if (ndotl > 0.0f) {
            const float g = geometrySmithIBL(ndotv, ndotl, roughness);
            const float gVis = (g * vdoth) / std::max(ndoth * ndotv, 1e-5f);
            const float fc = std::pow(1.0f - vdoth, 5.0f);
            a += (1.0f - fc) * gVis;
            b += fc * gVis;
        }
    }

    const float invSampleCount = 1.0f / static_cast<float>(sampleCount);
    return Vector3(a * invSampleCount, b * invSampleCount, 0.0f);
}

static bool loadHDR(const std::string& path, FloatImage& outImage, std::string& outError) {
    int32_t width = 0;
    int32_t height = 0;
    int32_t components = 0;
    float* raw = TextureFromFile::loadFloat(path.c_str(), &width, &height, &components, 3);
    if (!raw) {
        outError = "failed to load HDR image: " + path;
        return false;
    }

    outImage.width = width;
    outImage.height = height;
    outImage.pixels.resize(static_cast<size_t>(width * height));
    for (int i = 0; i < width * height; ++i) {
        outImage.pixels[static_cast<size_t>(i)] = Vector3(raw[i * 3 + 0], raw[i * 3 + 1], raw[i * 3 + 2]);
    }
    TextureFromFile::free(raw);
    return true;
}

static std::vector<float> flattenFloatRGB(const FloatImage& image) {
    std::vector<float> data(static_cast<size_t>(image.width * image.height * 3));
    for (int i = 0; i < image.width * image.height; ++i) {
        const Vector3& c = image.pixels[static_cast<size_t>(i)];
        data[static_cast<size_t>(i * 3 + 0)] = c.x;
        data[static_cast<size_t>(i * 3 + 1)] = c.y;
        data[static_cast<size_t>(i * 3 + 2)] = c.z;
    }
    return data;
}

static RGBA8Image encodeRGBM(const FloatImage& image, float maxRange) {
    RGBA8Image out(image.width, image.height);
    const float invMaxRange = 1.0f / std::max(maxRange, 1e-4f);

    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            const Vector3 hdr = image.get(x, y);
            const float maxChannel = std::max(hdr.x, std::max(hdr.y, hdr.z));
            float multiplier = std::ceil(saturate(maxChannel * invMaxRange) * 255.0f) / 255.0f;
            multiplier = std::max(multiplier, 1.0f / 255.0f);
            Vector3 rgb = hdr / (multiplier * maxRange);
            rgb.clamp(0.0f, 1.0f);
            out.set(x,
                    y,
                    static_cast<uint8_t>(std::round(saturate(rgb.x) * 255.0f)),
                    static_cast<uint8_t>(std::round(saturate(rgb.y) * 255.0f)),
                    static_cast<uint8_t>(std::round(saturate(rgb.z) * 255.0f)),
                    static_cast<uint8_t>(std::round(saturate(multiplier) * 255.0f)));
        }
    }
    return out;
}

static RGBA8Image encodeBRDFLUT(const FloatImage& image) {
    RGBA8Image out(image.width, image.height);
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            const Vector3 value = image.get(x, y);
            out.set(x,
                    y,
                    static_cast<uint8_t>(std::round(saturate(value.x) * 255.0f)),
                    static_cast<uint8_t>(std::round(saturate(value.y) * 255.0f)),
                    0,
                    255);
        }
    }
    return out;
}

static bool savePNG(const std::string& path, const RGBA8Image& image, std::string& outError) {
    if (!ensureParentDirectory(path)) {
        outError = "failed to create output directory for " + path;
        return false;
    }
    if (!TextureFromFile::save(path.c_str(), image.width, image.height, 4, image.pixels.data(), 0, false)) {
        outError = "failed to save png: " + path;
        return false;
    }
    return true;
}

static bool saveHDR(const std::string& path, const FloatImage& image, std::string& outError) {
    if (!ensureParentDirectory(path)) {
        outError = "failed to create output directory for " + path;
        return false;
    }
    const std::vector<float> data = flattenFloatRGB(image);
    if (!TextureFromFile::saveHdr(path.c_str(), image.width, image.height, 3, data.data(), false)) {
        outError = "failed to save hdr: " + path;
        return false;
    }
    return true;
}

static int computeSpecularAtlasHeight(int baseHeight, int mipCount, std::vector<int>& mipHeights, std::vector<int>& offsetsY) {
    mipHeights.clear();
    offsetsY.clear();

    int totalHeight = 0;
    int currentHeight = baseHeight;
    for (int level = 0; level < mipCount; ++level) {
        currentHeight = std::max(1, (level == 0) ? baseHeight : (currentHeight / 2));
        mipHeights.push_back(currentHeight);
        offsetsY.push_back(totalHeight);
        totalHeight += currentHeight;
    }
    return totalHeight;
}

static void writeOutputReadme(const IBLPrecomputeOptions& options, const IBLPrecomputeResult& result) {
    const std::string readmePath = joinPath(result.outputDirectory, "README.md");
    if (!ensureParentDirectory(readmePath)) {
        return;
    }

    std::ofstream out(readmePath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        return;
    }

    out << "# Offline IBL Bake Output\n\n";
    out << "Source HDR: `" << normalizePath(result.resolvedInputHDRPath) << "`\n\n";
    out << "## Files\n";
    out << "- `irradiance_equirect.png`: RGBM encoded diffuse irradiance equirect map\n";
    out << "- `specular_equirect.png`: RGBM encoded GGX prefiltered specular atlas\n";
    out << "- `brdf_lut.png`: split-sum BRDF LUT, BRDF.x in R, BRDF.y in G\n\n";
    out << "## RGBM decode\n";
    out << "```glsl\n";
    out << "vec3 decodeRGBM(vec4 rgbm) {\n";
    out << "    return rgbm.rgb * (rgbm.a * " << options.rgbmRange << ");\n";
    out << "}\n";
    out << "```\n\n";
    out << "## Specular atlas layout\n";
    out << "Mip levels are stacked from top to bottom.\n\n";
    out << "| mip | roughness | width | height | offsetY |\n";
    out << "| --- | --------- | ----- | ------ | ------- |\n";
    for (size_t i = 0; i < result.specularMipWidths.size(); ++i) {
        const float roughness = (result.specularMipWidths.size() <= 1)
                                    ? 0.0f
                                    : static_cast<float>(i) / static_cast<float>(result.specularMipWidths.size() - 1);
        out << "| " << i << " | " << roughness << " | " << result.specularMipWidths[i] << " | "
            << result.specularMipHeights[i] << " | " << result.specularMipOffsetsY[i] << " |\n";
    }
}

static void writeOutputMetadata(const IBLPrecomputeOptions& options, const IBLPrecomputeResult& result) {
    const std::string metaPath = joinPath(result.outputDirectory, "ibl_meta.txt");
    if (!ensureParentDirectory(metaPath)) {
        return;
    }

    std::ofstream out(metaPath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        return;
    }

    out << "rgbmRange=" << options.rgbmRange << "\n";
    out << "mipCount=" << result.specularMipWidths.size() << "\n";

    out << "specularMipWidths=";
    for (size_t i = 0; i < result.specularMipWidths.size(); ++i) {
        if (i > 0) out << ",";
        out << result.specularMipWidths[i];
    }
    out << "\n";

    out << "specularMipHeights=";
    for (size_t i = 0; i < result.specularMipHeights.size(); ++i) {
        if (i > 0) out << ",";
        out << result.specularMipHeights[i];
    }
    out << "\n";

    out << "specularMipOffsetsY=";
    for (size_t i = 0; i < result.specularMipOffsetsY.size(); ++i) {
        if (i > 0) out << ",";
        out << result.specularMipOffsetsY[i];
    }
    out << "\n";
}

} // namespace

bool IBLPrecompute::bake(const IBLPrecomputeOptions& options, IBLPrecomputeResult& outResult, std::string& outError) {
    if (options.inputHDRPath.empty()) {
        outError = "input HDR path is empty";
        return false;
    }

    IBLPrecomputeOptions resolved = options;
    resolved.inputHDRPath = normalizePath(resolved.inputHDRPath);
    resolved.outputDirectory = normalizePath(resolved.outputDirectory);
    if (resolved.outputDirectory.empty()) {
        resolved.outputDirectory = joinPath("../assets/textures/ibl", fileStem(resolved.inputHDRPath));
    }

    FloatImage environment;
    if (!loadHDR(resolved.inputHDRPath, environment, outError)) {
        return false;
    }

    outResult = {};
    outResult.resolvedInputHDRPath = resolved.inputHDRPath;
    outResult.outputDirectory = resolved.outputDirectory;
    outResult.irradiancePNGPath = joinPath(resolved.outputDirectory, "irradiance_equirect.png");
    outResult.specularPNGPath = joinPath(resolved.outputDirectory, "specular_equirect.png");
    outResult.brdfLUTPNGPath = joinPath(resolved.outputDirectory, "brdf_lut.png");
    outResult.irradianceHDRPath = joinPath(resolved.outputDirectory, "irradiance_equirect.hdr");
    outResult.specularHDRPath = joinPath(resolved.outputDirectory, "specular_equirect.hdr");

    FloatImage irradiance;
    irradiance.width = std::max(1, resolved.irradianceWidth);
    irradiance.height = std::max(1, resolved.irradianceHeight);
    irradiance.pixels.resize(static_cast<size_t>(irradiance.width * irradiance.height));
    for (int y = 0; y < irradiance.height; ++y) {
        for (int x = 0; x < irradiance.width; ++x) {
            const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(irradiance.width);
            const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(irradiance.height);
            irradiance.set(x, y, integrateIrradiance(environment, directionFromEquirect(u, v), std::max(1, resolved.irradianceSampleCount)));
        }
    }

    outResult.specularAtlasWidth = std::max(1, resolved.specularBaseWidth);
    outResult.specularAtlasHeight = computeSpecularAtlasHeight(std::max(1, resolved.specularBaseHeight),
                                                               std::max(1, resolved.specularMipCount),
                                                               outResult.specularMipHeights,
                                                               outResult.specularMipOffsetsY);
    outResult.specularMipWidths.clear();
    FloatImage specularAtlas;
    specularAtlas.width = outResult.specularAtlasWidth;
    specularAtlas.height = outResult.specularAtlasHeight;
    specularAtlas.pixels.assign(static_cast<size_t>(specularAtlas.width * specularAtlas.height), Vector3(0.0f, 0.0f, 0.0f));

    int mipWidth = std::max(1, resolved.specularBaseWidth);
    for (int mip = 0; mip < std::max(1, resolved.specularMipCount); ++mip) {
        mipWidth = std::max(1, (mip == 0) ? resolved.specularBaseWidth : (mipWidth / 2));
        const int mipHeight = outResult.specularMipHeights[static_cast<size_t>(mip)];
        const int offsetY = outResult.specularMipOffsetsY[static_cast<size_t>(mip)];
        outResult.specularMipWidths.push_back(mipWidth);
        const float roughness = (resolved.specularMipCount <= 1)
                                    ? 0.0f
                                    : static_cast<float>(mip) / static_cast<float>(resolved.specularMipCount - 1);

        for (int y = 0; y < mipHeight; ++y) {
            for (int x = 0; x < mipWidth; ++x) {
                const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(mipWidth);
                const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(mipHeight);
                const Vector3 reflectionDir = directionFromEquirect(u, v);
                specularAtlas.set(x, offsetY + y,
                                  prefilterSpecular(environment,
                                                    reflectionDir,
                                                    roughness,
                                                    std::max(1, resolved.specularSampleCount)));
            }
        }
    }

    FloatImage brdfLUT;
    brdfLUT.width = std::max(1, resolved.brdfLUTSize);
    brdfLUT.height = std::max(1, resolved.brdfLUTSize);
    brdfLUT.pixels.resize(static_cast<size_t>(brdfLUT.width * brdfLUT.height));
    for (int y = 0; y < brdfLUT.height; ++y) {
        const float roughness = (static_cast<float>(y) + 0.5f) / static_cast<float>(brdfLUT.height);
        for (int x = 0; x < brdfLUT.width; ++x) {
            const float ndotv = (static_cast<float>(x) + 0.5f) / static_cast<float>(brdfLUT.width);
            brdfLUT.set(x, y, integrateBRDF(std::max(ndotv, 1e-4f), roughness, std::max(1, resolved.brdfSampleCount)));
        }
    }

    if (!savePNG(outResult.irradiancePNGPath, encodeRGBM(irradiance, resolved.rgbmRange), outError)) {
        return false;
    }
    if (!savePNG(outResult.specularPNGPath, encodeRGBM(specularAtlas, resolved.rgbmRange), outError)) {
        return false;
    }
    if (!savePNG(outResult.brdfLUTPNGPath, encodeBRDFLUT(brdfLUT), outError)) {
        return false;
    }

    if (resolved.writeHDRDebugImages) {
        if (!saveHDR(outResult.irradianceHDRPath, irradiance, outError)) {
            return false;
        }
        if (!saveHDR(outResult.specularHDRPath, specularAtlas, outError)) {
            return false;
        }
    } else {
        outResult.irradianceHDRPath.clear();
        outResult.specularHDRPath.clear();
    }

    writeOutputReadme(resolved, outResult);
    writeOutputMetadata(resolved, outResult);
    return true;
}

} // namespace morrow


