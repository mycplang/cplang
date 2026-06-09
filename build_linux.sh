#!/bin/bash
# CP语言 跨平台构建脚本
# 用法:
#   桌面 Linux:  ./build_linux.sh
#   指定类型:    ./build_linux.sh Debug
#   只编编译器:  ./build_linux.sh Release cli
#   Android NDK: ANDROID_NDK=/path/to/ndk ./build_linux.sh android

set -e

BUILD_TYPE="${1:-Release}"
TARGET="${2:-all}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_MODE="linux"

# Android 模式检测
if [ "${1}" = "android" ]; then
    BUILD_MODE="android"
    BUILD_TYPE="${2:-Release}"
    TARGET="${3:-cplang_android}"
fi

echo "=== CP语言 构建 ==="
echo "构建模式: ${BUILD_MODE}"
echo "源码目录: ${SCRIPT_DIR}"
echo "构建类型: ${BUILD_TYPE}"
echo "目标:     ${TARGET}"
echo ""

# ═══════════════════════════════════════════════
#  Android NDK 交叉编译
# ═══════════════════════════════════════════════
if [ "${BUILD_MODE}" = "android" ]; then
    if [ -z "${ANDROID_NDK}" ]; then
        for ndk in "${HOME}/Android/Sdk/ndk"/* "${ANDROID_HOME}/ndk-bundle" "/usr/local/lib/android/sdk/ndk"*; do
            [ -d "$ndk" ] && { ANDROID_NDK="$ndk"; break; }
        done
    fi
    if [ -z "${ANDROID_NDK}" ] || [ ! -d "${ANDROID_NDK}" ]; then
        echo "错误: 未找到 Android NDK，设置 ANDROID_NDK 环境变量"
        exit 1
    fi
    echo "NDK: ${ANDROID_NDK}"
    TOOLCHAIN="${ANDROID_NDK}/build/cmake/android.toolchain.cmake"
    [ ! -f "$TOOLCHAIN" ] && { echo "错误: 缺少 $TOOLCHAIN"; exit 1; }

    BUILD_DIR="${SCRIPT_DIR}/build_android"
    mkdir -p "$BUILD_DIR"
    cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
        -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
        -DANDROID_ABI=arm64-v8a \
        -DANDROID_PLATFORM=android-24 \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCPLANG_BUILD_TESTS=OFF \
        -DCPLANG_BUILD_EXAMPLES=OFF
    cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --target "$TARGET" -j$(nproc 2>/dev/null || echo 4)
    echo ""
    echo "=== Android 构建结果 ==="
    find "$BUILD_DIR" -name "*.so" -exec ls -lh {} \;
    exit 0
fi

# ═══════════════════════════════════════════════
#  Linux 桌面构建
# ═══════════════════════════════════════════════
for cmd in cmake ninja g++ llvm-config; do
    if ! command -v $cmd &>/dev/null; then
        echo "错误: 缺少 $cmd"
        exit 1
    fi
done
echo "LLVM: $(llvm-config --version)"
echo "CMake: $(cmake --version | head -1)"
echo ""

BUILD_DIR="${SCRIPT_DIR}/build_linux"

if [ ! -f "${BUILD_DIR}/build.ninja" ] || [ "${SCRIPT_DIR}/CMakeLists.txt" -nt "${BUILD_DIR}/build.ninja" ]; then
    echo "--- 配置 CMake ---"
    mkdir -p "$BUILD_DIR"
    cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCPLANG_USE_LLVM=ON \
        -DCPLANG_BUILD_TESTS=OFF \
        -DCPLANG_BUILD_EXAMPLES=OFF \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    echo "配置完成"
else
    echo "--- 使用已有 CMake 配置 ---"
fi

echo "--- 构建 ---"
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --target "$TARGET" -j$(nproc 2>/dev/null || echo 4)

echo ""
echo "=== 构建结果 ==="
if [ -f "${BUILD_DIR}/bin/cplang" ]; then
    echo "✓ cplang: $(du -h ${BUILD_DIR}/bin/cplang | cut -f1)"
    echo "--- 运行测试 ---"
    ${BUILD_DIR}/bin/cplang -c ${SCRIPT_DIR}/examples/hello.cp
    echo ""
    echo "✓ VM 执行测试通过"
else
    echo "✗ cplang 编译器未生成"
    exit 1
fi
echo ""
echo "=== 构建成功 ==="
echo "产物: ${BUILD_DIR}/bin/"