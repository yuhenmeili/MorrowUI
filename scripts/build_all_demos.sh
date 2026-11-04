#!/bin/bash
# ============================================================
# QNX 交叉编译脚本 — 编译 QianliRender 及所有 Demo
# 用法:
#   ./build_all_demos.sh              # Debug 模式编译所有 demo
#   ./build_all_demos.sh Release      # Release 模式编译所有 demo
#   ./build_all_demos.sh Debug clean  # 先 clean 再编译
#
# 可选环境变量:
#   QNX_SDK_ENV   qnxsdp-env.sh 路径（默认 ~/workspace/sdp710）
#   QNX_ARCH      目标架构（默认 aarch64le，可选 x86_64）
# 若 shell 已 source 过 qnxsdp-env.sh（QNX_HOST/QNX_TARGET 已设置），跳过自动 source。
# ============================================================
set -e

# ---------- 配置 ----------
BUILD_TYPE="${1:-Debug}"
CLEAN_FIRST="${2:-}"
QNX_ARCH="${QNX_ARCH:-aarch64le}"

# 工程根目录 = 脚本所在目录的上一级（脚本位于 scripts/ 子目录）
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"

# ---------- QNX SDK 环境 ----------
# QNX 交叉编译器强依赖 QNX_HOST / QNX_TARGET 两个环境变量，
# 两者任一缺失时自动 source SDK 环境脚本。
if [ -z "${QNX_HOST}" ] || [ -z "${QNX_TARGET}" ]; then
    QNX_SDK_ENV="${QNX_SDK_ENV:-${HOME}/workspace/sdp710/qnxsdp-env.sh}"
    if [ ! -f "${QNX_SDK_ENV}" ]; then
        echo "[ERROR] 未找到 QNX SDK 环境脚本: ${QNX_SDK_ENV}"
        echo "        请先手动 source qnxsdp-env.sh，或用 QNX_SDK_ENV=... 指定路径"
        exit 1
    fi
    echo "[INFO] 自动加载 QNX SDK 环境: ${QNX_SDK_ENV}"
    # shellcheck disable=SC1090
    source "${QNX_SDK_ENV}" >/dev/null
fi

QCC="${QNX_HOST}/usr/bin/qcc"
QXX="${QNX_HOST}/usr/bin/q++"

# ---------- 环境检查 ----------
if [ ! -x "${QXX}" ]; then
    echo "[ERROR] QNX 编译器未找到: ${QXX}"
    echo "        QNX_HOST=${QNX_HOST}（检查 SDK 安装路径）"
    exit 1
fi

echo "============================================"
echo "  MorrowUI Demo 编译脚本（QNX ${QNX_ARCH}）"
echo "  构建类型: ${BUILD_TYPE}"
echo "  编译器:   ${QXX}"
echo "  构建目录: ${BUILD_DIR}"
echo "============================================"

# ---------- Clean（可选）----------
if [ "${CLEAN_FIRST}" = "clean" ]; then
    echo ""
    echo "[STEP] 清理构建目录..."
    rm -rf "${BUILD_DIR}"
    echo "[INFO] 构建目录已清理"
fi

# ---------- 缓存一致性检查 ----------
# 换编译器（如裸 gcc 换 qcc）或换目标架构后，旧 CMake 缓存无法复用，
# CMake 会报 "CMAKE_CXX_COMPILER changed" / 残留旧 -Vgcc_nto<arch> 旗标，直接重建。
if [ -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    CACHED_CXX="$(grep -m1 '^CMAKE_CXX_COMPILER:' "${BUILD_DIR}/CMakeCache.txt" | cut -d= -f2- || true)"
    CACHED_ARCH="$(grep -m1 '^CMAKE_SYSTEM_PROCESSOR:' "${BUILD_DIR}/CMakeCache.txt" | cut -d= -f2- || true)"
    if { [ -n "${CACHED_CXX}" ] && [ "${CACHED_CXX}" != "${QXX}" ]; } \
       || { [ -n "${CACHED_ARCH}" ] && [ "${CACHED_ARCH}" != "${QNX_ARCH}" ]; }; then
        echo "[WARN] 构建缓存与当前工具链/架构不一致（compiler=${CACHED_CXX}, arch=${CACHED_ARCH}），自动重建..."
        rm -rf "${BUILD_DIR}"
    fi
fi

# ---------- CMake 配置 ----------
# QNX 官方 CMake 用法：qcc/q++ 驱动 + 显式声明目标系统与架构。
# 由此顶层 CMakeLists 的 QNX 分支自动取到正确的 -Vgcc_nto${QNX_ARCH} 变体旗标。
echo ""
echo "[STEP] CMake 配置..."
cmake \
    -G "Unix Makefiles" \
    -DCMAKE_SYSTEM_NAME=QNX \
    -DCMAKE_SYSTEM_PROCESSOR="${QNX_ARCH}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_C_COMPILER="${QCC}" \
    -DCMAKE_CXX_COMPILER="${QXX}" \
    -B "${BUILD_DIR}" \
    -S "${PROJECT_DIR}"
echo "[INFO] CMake 配置完成"

# ---------- 编译所有 Demo ----------
echo ""
echo "[STEP] 编译所有 Demo..."
cmake --build "${BUILD_DIR}" --target all -- -j"$(nproc)"

echo ""
echo "============================================"
echo "  编译完成！"
echo "  可执行文件位于: ${BUILD_DIR}/"
echo "  资产目录:       ${BUILD_DIR}/assets/（部署时与可执行文件同目录）"
echo "============================================"

# 列出生成的可执行文件（demo 在顶层 CMakeLists 生成，产物直接位于 build 根目录）
echo ""
echo "生成的 Demo 可执行文件:"
find "${BUILD_DIR}" -maxdepth 1 -type f -executable | sort | while read -r f; do
    echo "  $(basename "$f")"
done
