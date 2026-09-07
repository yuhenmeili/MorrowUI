//
// Created by lance on 2026/9/7.
//

#include "FontUtils.h"

#include <algorithm>
#include <cmath>

namespace morrow {
namespace FontUtils {
void edt1d(const std::vector<int>& f, int n, std::vector<int>& d) {
    std::vector<int> v(n);
    std::vector<long long> z(n + 1);
    int k = 0;
    v[0] = 0;
    z[0] = -kEdtInf;
    z[1] = kEdtInf;
    for (int q = 1; q < n; ++q) {
        if (f[q] >= kEdtInf)
            continue;  // INF 抛物线不会进入 lower envelope
        long long s = 0;
        while (true) {
            const long long fq = static_cast<long long>(f[q]) + static_cast<long long>(q) * q;
            const long long fv = static_cast<long long>(f[v[k]]) + static_cast<long long>(v[k]) * v[k];
            s = (fq - fv) / (2 * (q - v[k]));
            if (s > z[k])
                break;
            --k;
        }
        ++k;
        v[k] = q;
        z[k] = s;
        z[k + 1] = kEdtInf;
    }
    k = 0;
    for (int q = 0; q < n; ++q) {
        while (z[k + 1] < q)
            ++k;
        const long long delta = q - v[k];
        d[q] = static_cast<int>(delta * delta + f[v[k]]);
    }
}
void edt2d(std::vector<int>& grid, int width, int height) {
    std::vector<int> f(std::max(width, height));
    std::vector<int> d(std::max(width, height));
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y)
            f[y] = grid[y * width + x];
        edt1d(f, height, d);
        for (int y = 0; y < height; ++y)
            grid[y * width + x] = d[y];
    }
    for (int y = 0; y < height; ++y) {
        int* row = grid.data() + y * width;
        for (int x = 0; x < width; ++x)
            f[x] = row[x];
        edt1d(f, width, d);
        for (int x = 0; x < width; ++x)
            row[x] = d[x];
    }
}
void encodeSdfFromCoverage(const unsigned char* coverage, int width, int height, int onedge, float pixelDistScale, std::vector<unsigned char>& out) {
    const size_t count = static_cast<size_t>(width) * height;
    // 距离场语义 = 到"源集合"（初始化为 0 的像素）的最近距离：
    // distOutside 的源是 outside 像素 → inside 像素取值 = 到边缘的深度；
    // distInside  的源是 inside 像素 → outside 像素取值 = 到字形的距离。
    std::vector<int> distOutside(count, kEdtInf);
    std::vector<int> distInside(count, kEdtInf);
    for (size_t i = 0; i < count; ++i) {
        if (coverage[i] >= 128) {
            distInside[i] = 0;
        } else {
            distOutside[i] = 0;
        }
    }
    edt2d(distOutside, width, height);
    edt2d(distInside, width, height);

    out.resize(count);
    for (size_t i = 0; i < count; ++i) {
        // 亚像素校正：二值化（阈值 128）把边缘量化到整像素台阶会产生锯齿，
        // 用原始 coverage 的覆盖率恢复边缘像素的真实亚像素位置——
        // 1D 近似下覆盖 50%+x% 的像素，其中心距边缘约 |x| px（x = cov - 0.5）：
        //   inside: d = D_out - 1.5 + cov   （D_out=1、cov=0.78 → 0.28px；深处 cov=1 → D_out-0.5）
        //   outside: d = -(D_in - 0.5) + cov（D_in=1、cov=0.25 → -0.25px；远处 cov=0 → -(D_in-0.5)）
        const float coverageNorm = coverage[i] / 255.0f;
        float d;
        if (coverage[i] >= 128) {
            d = std::sqrt(static_cast<float>(distOutside[i])) - 1.5f + coverageNorm;  // inside：到边缘深度
        } else {
            d = -(std::sqrt(static_cast<float>(distInside[i])) - 0.5f) + coverageNorm;  // outside：到边缘负距离
        }
        const float value = static_cast<float>(onedge) + d * pixelDistScale;
        out[i] = static_cast<unsigned char>(std::clamp(std::lround(value), 0L, 255L));
    }
}
}  // namespace FontUtils
}  // namespace morrow