//
// Created by lance on 2026/9/7.
//

#include "FontUtils.h"

#include <algorithm>
#include <cmath>

namespace morrow {
namespace FontUtils {
namespace {

// edt1d 的实现，可选输出每个采样点的最近源下标（输出时 lower envelope 上
// 胜出抛物线的站点 v[k] 即最近源）。srcOut 可为 nullptr。
void edt1dImpl(const std::vector<int>& f, int n, std::vector<int>& d, std::vector<int>* srcOut) {
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
        if (srcOut)
            (*srcOut)[q] = v[k];
    }
}

// edt2d + 最近源追踪：srcOut[i] = i 的最近源纹素下标（初始化为 0 的纹素）。
// 两遍 1D 组合：第一遍按列记录每列最近源行号，第二遍胜出列 × 该列源行号。
void edt2dWithSource(std::vector<int>& grid, int width, int height, std::vector<int>& srcOut) {
    std::vector<int> f(std::max(width, height));
    std::vector<int> d(std::max(width, height));
    std::vector<int> src1d(std::max(width, height));
    std::vector<int> srcRow(static_cast<size_t>(width) * height);
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y)
            f[y] = grid[y * width + x];
        edt1dImpl(f, height, d, &src1d);
        for (int y = 0; y < height; ++y) {
            grid[y * width + x] = d[y];
            srcRow[static_cast<size_t>(y) * width + x] = src1d[y];
        }
    }
    srcOut.assign(static_cast<size_t>(width) * height, 0);
    for (int y = 0; y < height; ++y) {
        int* row = grid.data() + y * width;
        for (int x = 0; x < width; ++x)
            f[x] = row[x];
        edt1dImpl(f, width, d, &src1d);
        for (int x = 0; x < width; ++x) {
            row[x] = d[x];
            // 最近源位于（列 = 胜出列 srcCol，行 = 该列在行 y 的最近源行号）
            const int srcCol = src1d[x];
            srcOut[static_cast<size_t>(y) * width + x] =
                static_cast<size_t>(srcRow[static_cast<size_t>(y) * width + srcCol]) * width + srcCol;
        }
    }
}

// 从最近源纹素 source 朝 target 方向迈一步，得到与 source 相邻、与 target
// 同类的边界纹素。主轴（|Δ| 大者，平局取 x）优先，落错类时换副轴：若两轴
// 都落在源类，则该侧存在比 source 更近的源，与"最近源"矛盾，故必有一轴
// 成功。distToOpposite = 源集合所在距离网格（非 0 即 target 同类）。
int boundaryTexel(int source, int target, int width, const std::vector<int>& distToOpposite) {
    const int dx = (target % width) - (source % width);
    const int dy = (target / width) - (source / width);
    const bool xFirst = std::abs(dx) >= std::abs(dy);
    for (int attempt = 0; attempt < 2; ++attempt) {
        const bool useX = (attempt == 0) == xFirst;
        const int stepX = useX ? (dx > 0) - (dx < 0) : 0;
        const int stepY = useX ? 0 : (dy > 0) - (dy < 0);
        if (stepX == 0 && stepY == 0)
            continue;  // 副轴无位移
        const int boundary = source + stepY * width + stepX;
        if (distToOpposite[boundary] != 0)
            return boundary;
    }
    return source;  // 理论不可达，防御
}

}  // namespace

void edt1d(const std::vector<int>& f, int n, std::vector<int>& d) {
    edt1dImpl(f, n, d, nullptr);
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
    std::vector<int> srcOutside;
    std::vector<int> srcInside;
    edt2dWithSource(distOutside, width, height, srcOutside);
    edt2dWithSource(distInside, width, height, srcInside);

    out.resize(count);
    for (size_t i = 0; i < count; ++i) {
        // 亚像素校正：EDT 的 D 是到对侧纹素中心的整纹素距离，真实边缘位于
        // 边界纹素对——inside 侧 b（cov∈[0.5,1]）与 outside 侧 s（cov∈[0,0.5]）——
        // 中覆盖率未饱和那一个的跨度内。整条距离剖面统一锚定该边界对的覆盖率：
        //   inside 纹素:  边在 b 内 → d = D_out - 1.5 + cov_b（D_out=1 即锚定自身，含此特例）
        //                 边在 s 内 → d = D_out - 0.5 + cov_s（cov_b 饱和为 1 时）
        //   outside 纹素: 边在外侧纹素内 → d = -(D_in - 0.5) + cov_out（自身，含此特例）
        //                 否则锚定内侧源 → d = -(D_in - 0.5) - (1 - cov_in)
        // 两式在双饱和（边恰在纹素界上）时相等。同一平直边缘上的所有纹素锚定
        // 同一纹素对，插值零点恰好落在真实边缘。
        // （若各纹素用自身覆盖率，饱和值 1/0 等效于假设边总在纹素界上，深层
        // 纹素把场整体推向内侧，笔画系统性偏厚 ~0.2px/边。）
        float d;
        if (coverage[i] >= 128) {
            const int source = srcOutside[i];
            const int boundary = boundaryTexel(source, static_cast<int>(i), width, distOutside);
            const float dist = std::sqrt(static_cast<float>(distOutside[i]));
            const float covBoundary = coverage[boundary] / 255.0f;
            const float covSource = coverage[source] / 255.0f;
            d = covBoundary < 1.0f ? dist - 1.5f + covBoundary : dist - 0.5f + covSource;
        } else {
            const int source = srcInside[i];
            const int boundary = boundaryTexel(source, static_cast<int>(i), width, distInside);
            const float dist = std::sqrt(static_cast<float>(distInside[i]));
            const float covBoundary = coverage[boundary] / 255.0f;
            const float covSource = coverage[source] / 255.0f;
            d = covBoundary > 0.0f ? -(dist - 0.5f) + covBoundary : -(dist - 0.5f) - (1.0f - covSource);
        }
        const float value = static_cast<float>(onedge) + d * pixelDistScale;
        out[i] = static_cast<unsigned char>(std::clamp(std::lround(value), 0L, 255L));
    }
}
}  // namespace FontUtils
}  // namespace morrow
