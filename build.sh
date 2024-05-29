#!/usr/bin/sh

NDK_PATH="/home/android-ndk"
BASE_DIR=$(dirname $0)
BUILD_DIR="${BASE_DIR}/build"
SOURCE_DIR="${BASE_DIR}/source"
INJECTOR_DIR="${BASE_DIR}/injector"
INJECTOR_BUILD_DIR="${INJECTOR_DIR}/build"
TOOLCHAIN_BIN="${NDK_PATH}/toolchains/llvm/prebuilt/linux-x86_64/bin"
TARGET_PREFIX="aarch64-linux-android31"

rm -rf ${BUILD_DIR}
mkdir -p ${BUILD_DIR}

cmake \
    -DCMAKE_BUILD_TYPE="release" \
    -DCMAKE_C_COMPILER="${TOOLCHAIN_BIN}/${TARGET_PREFIX}-clang" \
    -DCMAKE_CXX_COMPILER="${TOOLCHAIN_BIN}/${TARGET_PREFIX}-clang++" \
    -H${SOURCE_DIR} \
    -B${BUILD_DIR} \
    -G "Unix Makefiles"
cmake --build ${BUILD_DIR} --config "release" --target "CuJankDetector" -j16

rm -rf ${INJECTOR_BUILD_DIR}
mkdir -p ${INJECTOR_BUILD_DIR}

cmake \
    -DCMAKE_BUILD_TYPE="release" \
    -DCMAKE_C_COMPILER="${TOOLCHAIN_BIN}/${TARGET_PREFIX}-clang" \
    -DCMAKE_CXX_COMPILER="${TOOLCHAIN_BIN}/${TARGET_PREFIX}-clang++" \
    -H${INJECTOR_DIR} \
    -B${INJECTOR_BUILD_DIR} \
    -G "Unix Makefiles"
cmake --build ${INJECTOR_BUILD_DIR} --config "release" --target "injector" -j16

exit 0
