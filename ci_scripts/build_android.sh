#!/bin/bash
# Copyright 2020 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

set -e

# Fallback to arm64-v8a
ANDROID_ABI=${ANDROID_ABI:-'arm64-v8a'}

# You need to set the following environment variables first
# ANDROID_NDK= <Path to Android NDK> 

echo "Configure KTX-Software (Android $ANDROID_ABI Debug)"
cmake . -G Ninja -B "build-android-$ANDROID_ABI-debug" \
-DANDROID_PLATFORM=android-24 \
-DANDROID_ABI="$ANDROID_ABI" \
-DANDROID_NDK="$ANDROID_NDK" \
-DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
-DCMAKE_BUILD_TYPE=Debug

pushd "build-android-$ANDROID_ABI-debug"

echo "Build KTX-Software (Android $ANDROID_ABI Debug)"
cmake --build . --config Debug -j
# echo "Test KTX-Software (Android $ANDROID_ABI Debug)"
# ctest -C Debug # --verbose
echo "Install KTX-Software (Android $ANDROID_ABI Debug)"
cmake --install . --config Debug --prefix ../install-android-debug/$ANDROID_ABI

popd


echo "Configure KTX-Software (Android $ANDROID_ABI Release)"
cmake . -G Ninja -B "build-android-$ANDROID_ABI" \
-DANDROID_PLATFORM=android-24 \
-DANDROID_ABI=$ANDROID_ABI \
-DANDROID_NDK="$ANDROID_NDK" \
-DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
-DCMAKE_BUILD_TYPE=Release

pushd "build-android-$ANDROID_ABI"

echo "Build KTX-Software (Android $ANDROID_ABI Release)"
cmake --build . --config Release -j
# echo "Test KTX-Software (Android $ANDROID_ABI Release)"
# ctest -C Release # --verbose
echo "Install KTX-Software (Android $ANDROID_ABI Release)"
cmake --install . --config Release --prefix ../install-android/$ANDROID_ABI

popd
