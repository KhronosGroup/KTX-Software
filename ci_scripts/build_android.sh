#!/bin/bash
# Copyright 2020 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

set -e

# Fallback to arm64-v8a
ANDROID_ABI=${ANDROID_ABI:-'arm64-v8a'}
ASTC_ISA=${ASTC_ISA:-'ISA_NONE=ON'}
CONFIGURATION=${CONFIGURATION:-Release}

# You need to set the following environment variables first
# ANDROID_NDK= <Path to Android NDK>

echo "Configure KTX-Software (Android $ANDROID_ABI Release)"
cmake . -G Ninja -B "build-android-$ANDROID_ABI" \
  -D ANDROID_PLATFORM=android-24 \
  -D ANDROID_ABI="$ANDROID_ABI" \
  -D ANDROID_NDK="$ANDROID_NDK" \
  -D CMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
  -D CMAKE_BUILD_TYPE="$CONFIGURATION" \
  -D BASISU_SUPPORT_SSE=OFF \
  -D ${ASTC_ISA}

pushd "build-android-$ANDROID_ABI"

echo "Build KTX-Software (Android $ANDROID_ABI Release)"
cmake --build . --config $CONFIGURATION -j
# echo "Test KTX-Software (Android $ANDROID_ABI Release)"
# ctest --output-on-failure -C Release # --verbose
if [ "$CONFIGURATION" = "Release" ]; then
  echo "Install KTX-Software (Android $ANDROID_ABI Release)"
  cmake --install . --config Release --prefix ../install-android/$ANDROID_ABI
fi

popd
