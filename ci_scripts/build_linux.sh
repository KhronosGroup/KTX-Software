#! /usr/bin/env bash
# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Build on Linux.

######################################################################
#  Nota Bene
#
# Contains untested cross-compilation support that was under
# development when Travis-CI made arm64 Ubuntu runners available
# rendering it unneeded. Kept here to preserve the learning and in
# case it becomes useful.
######################################################################

# Exit if any command fails.
set -e

# cd repo root so script will work whereever the current directory
path_to_repo_root=..
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/$path_to_repo_root"

# Set parameters from command-line arguments, if any.
for i in $@; do
  eval $i
done

# Set some defaults
ARCH=${ARCH:-$(uname -m)}
CMAKE_GEN=${CMAKE_GEN:-Ninja Multi-Config}
CONFIGURATION=${CONFIGURATION:-Release}
FEATURE_DOC=${FEATURE_DOC:-OFF}
FEATURE_JNI=${FEATURE_JNI:-OFF}
if [ "$ARCH" = "x86_64" ]; then
  FEATURE_LOADTESTS=${FEATURE_LOADTESTS:-OpenGL+Vulkan}
else
  # No Vulkan SDK yet for Linux/arm64.
  FEATURE_LOADTESTS=${FEATURE_LOADTESTS:-OpenGL}
fi
FEATURE_PY=${FEATURE_PY:-OFF}
FEATURE_TESTS=${FEATURE_TESTS:-ON}
FEATURE_TOOLS=${FEATURE_TOOLS:-ON}
FEATURE_TOOLS_CTS=${FEATURE_TOOLS_CTS:-ON}
FEATURE_GL_UPLOAD=${FEATURE_GL_UPLOAD:-ON}
FEATURE_VK_UPLOAD=${FEATURE_VK_UPLOAD:-ON}
PACKAGE=${PACKAGE:-NO}
SUPPORT_SSE=${SUPPORT_SSE:-ON}
SUPPORT_OPENCL=${SUPPORT_OPENCL:-OFF}
WERROR=${WERROR:-OFF}

if [[ "$ARCH" = "aarch64" && "$FEATURE_LOADTESTS" =~ "Vulkan" ]]; then
  if [[ "$FEATURE_LOADTESTS" = "Vulkan" ]]; then
    FEATURE_LOADTESTS=OFF
  else
    FEATURE_LOADTESTS=OpenGL
  fi
  echo "Forcing FEATURE_LOADTESTS to \"$FEATURE_LOADTESTS\" as no Vulkan SDK yet for Linux/arm64."
fi

if [[ -z $BUILD_DIR ]]; then
  BUILD_DIR=build/linux
  if [ "$ARCH" != $(uname -m) ]; then
    BUILD_DIR+="-$ARCH-"
  fi
  if [ "$CMAKE_GEN" = "Ninja" -o "$CMAKE_GEN" = "Unix Makefiles" ]; then
    # Single configuration generators.
    BUILD_DIR+="-$CONFIGURATION"
  fi
fi

mkdir -p $BUILD_DIR

if [ "$FEATURE_TOOLS_CTS" = "ON" ]; then
  git submodule update --init --recursive tests/cts
fi

cmake_args=("-G" "$CMAKE_GEN"
  "-B" $BUILD_DIR \
  "-D" "CMAKE_BUILD_TYPE=$CONFIGURATION" \
  "-D" "KTX_FEATURE_DOC=$FEATURE_DOC" \
  "-D" "KTX_FEATURE_JNI=$FEATURE_JNI" \
  "-D" "KTX_FEATURE_LOADTEST_APPS=$FEATURE_LOADTESTS" \
  "-D" "KTX_FEATURE_PY=$FEATURE_PY" \
  "-D" "KTX_FEATURE_TESTS=$FEATURE_TESTS" \
  "-D" "KTX_FEATURE_TOOLS=$FEATURE_TOOLS" \
  "-D" "KTX_FEATURE_TOOLS_CTS=$FEATURE_TOOLS_CTS" \
  "-D" "KTX_FEATURE_GL_UPLOAD=$FEATURE_GL_UPLOAD" \
  "-D" "KTX_FEATURE_VK_UPLOAD=$FEATURE_VK_UPLOAD" \
  "-D" "BASISU_SUPPORT_OPENCL=$SUPPORT_OPENCL" \
  "-D" "BASISU_SUPPORT_SSE=$SUPPORT_SSE" \
  "-D" "KTX_WERROR=$WERROR"
)
if [ "$ARCH" != $(uname -m) ]; then
  cmake_args+=("--toolchain", "cmake/linux-$ARCH-toolchain.cmake")
fi
config_display="Configure KTX-Software (Linux on $ARCH): "
for arg in "${cmake_args[@]}"; do
  case $arg in
    "-A") config_display+="Arch=" ;;
    "-G") config_display+="Generator=" ;;
    "-B") config_display+="Build Dir=" ;;
    "-D") ;;
    "--toolchain") config_display+="Toolchain File=" ;;
    *) config_display+="$arg, " ;;
  esac
done
echo ${config_display%??}
cmake . "${cmake_args[@]}"

pushd $BUILD_DIR

oldifs=$IFS
#; is necessary because `for` is a Special Builtin.
IFS=, ; for config in $CONFIGURATION
do
  IFS=$oldifs # Because of ; IFS set above will still be present.
  # Build and test
  echo "Build KTX-Software (Linux $ARCH $config)"
  cmake --build . --config $config
  if [ "$ARCH" = "$(uname -m)" ]; then
    echo "Test KTX-Software (Linux $ARCH $config)"
    ctest --output-on-failure -C $config #--verbose
  fi
  if [ "$config" = "Release" -a "$PACKAGE" = "YES" ]; then
    echo "Pack KTX-Software (Linux $ARCH $config)"
    if ! cpack -C $config -G DEB; then
      # The DEB generator does not seem to write any log files.
      #cat _CPack_Packages/Linux/DEB/DEBOutput.log
      exit 1
    fi
    if ! cpack -C $config -G RPM; then
      echo "RPM generator .err file"
      cat _CPack_Packages/Linux/RPM/rpmbuildktx-software.err
      echo "RPM generator .out file"
      cat _CPack_Packages/Linux/RPM/rpmbuildktx-software.out
      exit 1
    fi
    if ! cpack -C $config -G TBZ2; then
      # The TBZ2 generator does not seem to write any log files.
      # cat _CPack_Packages/Linux/TBZ2/TBZ2Output.log
      exit 1
    fi
  fi
done

#echo "***** toktx version.h *****"
#cat tools/toktx/version.h
#echo "****** toktx version ******"
#build/linux-release/tools/toktx/toktx --version
#echo "***************************"

popd

# vim:ai:ts=4:sts=2:sw=2:expandtab

