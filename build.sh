#!/usr/bin/sh

NDK_PATH="/home/android-ndk"
BASE_DIR=$(dirname $0)
BUILD_DIR="${BASE_DIR}/build"
SOURCE_DIR="${BASE_DIR}/source"
TOOLCHAIN_BIN="${NDK_PATH}/toolchains/llvm/prebuilt/linux-x86_64/bin"
TARGET_PREFIX="aarch64-linux-android31"

rm -rf ${BUILD_DIR}
mkdir -p ${BUILD_DIR}

echo "- Build libCuJankDetector.so."
${TOOLCHAIN_BIN}/${TARGET_PREFIX}-clang++ \
    --shared "${SOURCE_DIR}/src/main.cpp" "${SOURCE_DIR}/dobby/libdobby.a" \
    -fPIC -static-libstdc++ -O3 -llog -fvisibility=hidden -w \
    -o "${BUILD_DIR}/libCuJankDetector.so"

echo "- Build injector."
${TOOLCHAIN_BIN}/${TARGET_PREFIX}-clang++ "${BASE_DIR}/inject/inject.cpp" -static-libstdc++ -O3 -w -o "${BUILD_DIR}/injector"

echo "- Done."
exit 0;
