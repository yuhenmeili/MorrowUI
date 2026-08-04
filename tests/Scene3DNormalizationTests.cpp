#include <cmath>
#include <iostream>
#include <string>

#include "ui/helpers/Scene3DNormalization.h"

using namespace morrow;
using namespace morrow::Math;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

void expectNear(float actual, float expected, const std::string& message) {
    constexpr float epsilon = 0.0001f;
    expect(std::fabs(actual - expected) <= epsilon, message);
}

void testSmallSceneIsScaledToTargetRadius() {
    Scene3DNormalizationOptions options;
    options.targetRadius = 2.5f;

    const auto result = calculateScene3DNormalization(
        Vector3(-0.01f, 0.0f, -0.025f),
        Vector3(0.01f, 0.015f, 0.025f),
        options);

    expect(result.applied, "a valid small scene should be normalized");
    expect(result.uniformScale > 1.0f,
           "a small scene should receive a scale larger than one");
    expectNear(result.normalizedRadius, options.targetRadius,
               "the normalized radius should match the configured target");
    expectNear((result.boundsMax - result.boundsMin).length() * 0.5f,
               options.targetRadius,
               "the returned bounds should include the normalization scale");
}

void testLargeSceneIsScaledDown() {
    Scene3DNormalizationOptions options;
    options.targetRadius = 2.5f;

    const auto result = calculateScene3DNormalization(
        Vector3(-10.0f, -10.0f, -10.0f),
        Vector3(10.0f, 10.0f, 10.0f),
        options);

    expect(result.applied, "a valid large scene should be normalized");
    expect(result.uniformScale < 1.0f,
           "a large scene should receive a scale smaller than one");
    expectNear(result.normalizedRadius, options.targetRadius,
               "a large scene should also reach the target radius");
}

void testDisabledNormalizationKeepsBounds() {
    Scene3DNormalizationOptions options;
    options.enabled = false;
    const Vector3 boundsMin(-1.0f, -2.0f, -3.0f);
    const Vector3 boundsMax(4.0f, 5.0f, 6.0f);

    const auto result =
        calculateScene3DNormalization(boundsMin, boundsMax, options);

    expect(!result.applied, "disabled normalization should not be applied");
    expectNear(result.uniformScale, 1.0f,
               "disabled normalization should keep unit scale");
    expect(result.boundsMin == boundsMin && result.boundsMax == boundsMax,
           "disabled normalization should preserve the original bounds");
}

void testDegenerateBoundsAreRejected() {
    Scene3DNormalizationOptions options;
    const Vector3 point(1.0f, 1.0f, 1.0f);

    const auto result =
        calculateScene3DNormalization(point, point, options);

    expect(!result.applied,
           "a zero-radius scene should not produce an extreme scale");
    expectNear(result.uniformScale, 1.0f,
               "a degenerate scene should keep unit scale");
}

void testScaleLimitsAreRespected() {
    Scene3DNormalizationOptions options;
    options.targetRadius = 2.5f;
    options.maxScale = 10.0f;

    const auto result = calculateScene3DNormalization(
        Vector3(-0.0005f, -0.0005f, -0.0005f),
        Vector3(0.0005f, 0.0005f, 0.0005f),
        options);

    expect(result.applied, "a small but valid scene should be normalized");
    expectNear(result.uniformScale, options.maxScale,
               "normalization should clamp to the configured maximum scale");
    expect(result.normalizedRadius < options.targetRadius,
           "a clamped scale may intentionally remain below the target radius");
}

} // namespace

int main() {
    testSmallSceneIsScaledToTargetRadius();
    testLargeSceneIsScaledDown();
    testDisabledNormalizationKeepsBounds();
    testDegenerateBoundsAreRejected();
    testScaleLimitsAreRespected();

    if (g_failures != 0) {
        std::cerr << g_failures << " scene normalization test(s) failed\n";
        return 1;
    }
    std::cout << "All scene normalization tests passed\n";
    return 0;
}
