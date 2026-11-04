#!/bin/bash
# ============================================================
# QNX 交叉编译脚本 — 编译单个 Demo
# 用法:
#   ./build_demo.sh TextDemo            # Debug 模式编译 TextDemo
#   ./build_demo.sh TextDemo Release    # Release 模式编译 TextDemo
#   ./build_demo.sh all                 # 编译所有 demo
#
# 可选环境变量:
#   QNX_SDK_ENV   qnxsdp-env.sh 路径（默认 ~/workspace/sdp710）
#   QNX_ARCH      目标架构（默认 aarch64le，可选 x86_64）
# 若 shell 已 source 过 qnxsdp-env.sh（QNX_HOST/QNX_TARGET 已设置），跳过自动 source。
# ============================================================
set -e

# ---------- 参数解析 ----------
TARGET="${1}"
BUILD_TYPE="${2:-Debug}"
QNX_ARCH="${QNX_ARCH:-aarch64le}"

# 工程根目录 = 脚本所在目录的上一级（脚本位于 scripts/ 子目录）
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"

# 可用目标从 samples/*.cpp 动态生成（目标名 = 文件名去后缀）
list_demos() {
    find "${PROJECT_DIR}/samples" -maxdepth 1 -name '*.cpp' -printf '%f\n' | sed 's/\.cpp$//' | sort
}

print_usage() {
    echo "用法: $0 <目标名> [构建类型(Debug|Release|RelWithDebInfo|MinSizeRel)]"
    echo ""
    echo "可用目标:"
    echo "  all                    编译所有 demo"
    for d in $(list_demos); do
        echo "  ${d}"
    done
    echo ""
    echo "示例:"
    echo "  $0 TextDemo            # Debug 编译 TextDemo"
    echo "  $0 TextDemo Release    # Release 编译 TextDemo"
    echo "  $0 all                 # 编译所有 demo"
}

if [ -z "${TARGET}" ]; then
    print_usage
    exit 0
fi

# 校验目标存在（all 除外）
if [ "${TARGET}" != "all" ] && ! list_demos | grep -qx -- "${TARGET}"; then
    echo "[ERROR] 目标不存在: ${TARGET}"
    echo ""
    print_usage
    exit 1
fi

# ---------- QNX SDK 环境 ----------
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
echo "  编译目标: ${TARGET}（QNX ${QNX_ARCH}）"
echo "  构建类型: ${BUILD_TYPE}"
echo "============================================"

# ---------- 缓存一致性检查 ----------
# 换编译器或换目标架构后旧缓存无法复用，直接重建（详见 build_all_demos.sh 同名段落）。
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
# 每次都重新 configure：开销极小（<1s），且能让 BUILD_TYPE 切换即时生效。
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

# ---------- 编译 ----------
echo ""
echo "[STEP] 编译 ${TARGET}..."
cmake --build "${BUILD_DIR}" --target "${TARGET}" -- -j"$(nproc)"

OUT_PATH="${BUILD_DIR}/${TARGET}"
echo ""
echo "============================================"
echo "  编译完成！"
echo "  可执行文件: ${OUT_PATH}"
echo "  资产目录:   ${BUILD_DIR}/assets/（部署时与可执行文件同目录）"
echo "============================================"
