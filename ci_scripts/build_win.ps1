# Copyright 2022 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Allow setting of variables on the command line. A command line parameter
# must look like (including the quotes shown) '$VAR="string"'. Spaces
# around the equals are acceptable.
for ($i=0; $i -lt $args.length; $i++)
{
   echo $($args[$i])
   Invoke-Expression $($args[$i])
}

# Setting env vars here sets them in environment for parent as well so
# use local variables to avoid pollution.
$BUILD_DIR `
  = $(if ($env:BUILD_DIR) { $env:BUILD_DIR} else { "build/build-batch-vs2019" })
$CONFIGURATION `
  = $(if ($env:CONFIGURATION) { $env:CONFIGURATION } else { "Release" })
$CMAKE_GEN `
  = $(if ($env:CMAKE_GEN) { $env:CMAKE_GEN } else { "Visual Studio 16 2019" })
$FEATURE_DOC = $(if ($env:FEATURE_DOC) { $env:FEATURE_DOC } else { "OFF" })
$FEATURE_JNI = $(if ($env:FEATURE_JNI) { $env:FEATURE_JNI } else { "OFF" })
$FEATURE_LOADTESTS `
  = $(if ($env:FEATURE_LOADTESTS) { $env:FEATURE_LOADTESTS } else { "OFF" })
$FEATURE_TOOLS = $(if ($env:FEATURE_TOOLS) { $env:FEATURE_TOOLS } else { "ON" })
$PLATFORM = $(if ($env:PLATFORM) { $env:PLATFORM } else { "x64" })
$PACKAGE = $(if ($env:PACKAGE) { $env:PACKAGE } else { "NO" })
$SUPPORT_SSE = $(if ($env:SUPPORT_SSE) { $env:SUPPORT_SSE } else { "OFF" })
$SUPPORT_OPENCL `
   = $(if ($env:SUPPORT_OPENCL) { $env:SUPPORT_OPENCL } else { "OFF" })
$OPENGL_ES_EMULATOR = $(if ($env:OPENGL_ES_EMULATOR) {
  $env:OPENGL_ES_EMULATOR
} else {
  "c:/Imagination` Technologies/PowerVR_Graphics/PowerVR_Tools/PVRVFrame/Library/Windows_x86_64"

})

if ($FEATURE_LOADTESTS -eq "ON")  { $need_gles_emulator=1 }

echo "Build via $CMAKE_GEN dir: $build_dir Arch: $PLATFORM Config: $CONFIGURATION, FEATURE_LOADTESTS: $FEATURE_LOADTESTS, FEATURE_DOC: $FEATURE_DOC, FEATURE_JNI: $FEATURE_JNI, FEATURE_TOOLS: $FEATURE_TOOLS, SUPPORT_SSE: $SUPPORT_SSE, SUPPORT_OPENCL: $SUPPORT_OPENCL"

cmake . -G "$CMAKE_GEN" -A $PLATFORM -B $BUILD_DIR `
  -D KTX_FEATURE_DOC=$FEATURE_DOC `
  -D KTX_FEATURE_JNI=$FEATURE_JNI `
  -D KTX_FEATURE_LOADTEST_APPS=$FEATURE_LOADTESTS `
  -D KTX_FEATURE_TOOLS=$FEATURE_TOOLS `
  -D BASISU_SUPPORT_SSE=$SUPPORT_SSE `
  -D BASISU_SUPPORT_OPENCL=$SUPPORT_OPENCL `
  $(if ($need_gles_emulator) {"-D"}) $(if ($need_gles_emulator) {"OPENGL_ES_EMULATOR=$OPENGL_ES_EMULATOR"})

pushd $BUILD_DIR
try {
  #git status
  cmake --build . --config $CONFIGURATION
  #git status
  if ($PACKAGE -eq "YES") {
    cmake --build . --config $CONFIGURATION --target PACKAGE
  }
  echo "Done building."
} finally {
  popd
}
