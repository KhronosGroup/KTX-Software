# Copyright 2022 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Allow setting of variables on the command line. A command line parameter
# must look like (including the quotes shown) '$VAR="string"'. Spaces
# around the equals are acceptable.
for ($i=0; $i -lt $args.length; $i++)
{
 Invoke-Expression $($args[$i])
}

function Set-Config-Variable {
  param ( $VariableName, $DefaultValue )
  $res = get-variable $VariableName -ValueOnly -ErrorAction 'SilentlyContinue'
  if ($res -eq $null) {
    $res = [Environment]::GetEnvironmentVariable($VariableName)
    if ($res -eq $null) {
        $res = $DefaultValue
    }
  } 
  return $res
}

# Setting env vars here sets them in environment for parent as well so
# use local variables to avoid pollution.
$BUILD_DIR = Set-Config-Variable BUILD_DIR "build/build-batch-vs2019"
$CONFIGURATION = Set-Config-Variable CONFIGURATION "Release"
$CMAKE_GEN = Set-Config-Variable CMAKE_GEN "Visual Studio 16 2019"
$FEATURE_DOC = Set-Config-Variable FEATURE_DOC "OFF"
$FEATURE_JNI = Set-Config-Variable FEATURE_JNI "OFF"
$FEATURE_LOADTESTS = Set-Config-Variable FEATURE_LOADTESTS "OFF"
$FEATURE_TOOLS = Set-Config-Variable FEATURE_TOOLS "ON"
$PLATFORM = Set-Config-Variable PLATFORM "x64"
$PACKAGE = Set-Config-Variable PACKAGE "NO"
$SUPPORT_SSE = Set-Config-Variable SUPPORT_SSE "ON"
$SUPPORT_OPENCL = Set-Config-Variable SUPPORT_OPENCL "OFF"
$OPENGL_ES_EMULATOR = Set-Config-Variable OPENGL_ES_EMULATOR `
  "c:/Imagination` Technologies/PowerVR_Graphics/PowerVR_Tools/PVRVFrame/Library/Windows_x86_64"
$WINDOWS_CODE_SIGN_IDENTITY = Set-Config-Variable WINDOWS_CODE_SIGN_IDENTITY ""

if ($FEATURE_LOADTESTS -eq "ON")  { $need_gles_emulator=1 }

echo "Build via $CMAKE_GEN dir: $build_dir Arch: $PLATFORM Config: $CONFIGURATION, FEATURE_LOADTESTS: $FEATURE_LOADTESTS, FEATURE_DOC: $FEATURE_DOC, FEATURE_JNI: $FEATURE_JNI, FEATURE_TOOLS: $FEATURE_TOOLS, SUPPORT_SSE: $SUPPORT_SSE, SUPPORT_OPENCL: $SUPPORT_OPENCL WINDOWS_CODE_SIGN_IDENTITY: ${WINDOWS_CODE_SIGN_IDENTITY}"

cmake . -G "$CMAKE_GEN" -A $PLATFORM -B $BUILD_DIR `
  -D KTX_FEATURE_DOC=$FEATURE_DOC `
  -D KTX_FEATURE_JNI=$FEATURE_JNI `
  -D KTX_FEATURE_LOADTEST_APPS=$FEATURE_LOADTESTS `
  -D KTX_FEATURE_TOOLS=$FEATURE_TOOLS `
  -D BASISU_SUPPORT_SSE=$SUPPORT_SSE `
  -D BASISU_SUPPORT_OPENCL=$SUPPORT_OPENCL `
  -D WINDOWS_CODE_SIGN_IDENTITY=$WINDOWS_CODE_SIGN_IDENTITY `
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
