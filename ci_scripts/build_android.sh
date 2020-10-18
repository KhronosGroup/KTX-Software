#!/bin/bash

set -e

# You need to set the following environment variables first
# ANDROID_NDK= <Path to Android NDK> 
# ZSTD_PATH= <Path to ZStandard sources>

echo "Configure KTX-Software (Android arm64-v8a Debug)"
cmake . -B build-android-arm64-v8a \
-DANDROID_PLATFORM=android-24 \
-DANDROID_ABI=arm64-v8a \
-DANDROID_NDK="$ANDROID_NDK" \
-DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
-DCMAKE_BUILD_TYPE=Debug \
-DZSTD_PATH="$ZSTD_PATH"

pushd build-android-arm64-v8a

echo "Build KTX-Software (Android arm64-v8a Debug)"
cmake --build . --config Debug -j 8
# echo "Test KTX-Software (Android arm64-v8a Debug)"
# ctest -C Debug # --verbose
echo "Install KTX-Software (Android arm64-v8a Debug)"
cmake --install . --config Debug --prefix ../install-android-arm64-v8a

popd


echo "Configure KTX-Software (Android arm64-v8a Release)"
cmake . -B build-android-arm64-v8a-release \
-DANDROID_PLATFORM=android-24 \
-DANDROID_ABI=arm64-v8a \
-DANDROID_NDK="$ANDROID_NDK" \
-DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
-DCMAKE_BUILD_TYPE=Release \
-DZSTD_PATH="$ZSTD_PATH"

pushd build-android-arm64-v8a-release

echo "Build KTX-Software (Android arm64-v8a Release)"
cmake --build . --config Release -j 8
# echo "Test KTX-Software (Android arm64-v8a Release)"
# ctest -C Release # --verbose
echo "Install KTX-Software (Android arm64-v8a Release)"
cmake --install . --config Release --prefix ../install-android-arm64-v8a-release

popd
