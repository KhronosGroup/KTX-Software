# Copyright 2022 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

#   Allow setting of variables on the command line. A command line parameter
# must look like (including the quotes shown) '$VAR="string"'. Spaces
# around the equals are acceptable.
#   Since the cmake Visual Studio generator generates multi-config build
# trees, CONFIGURATION can be a comma separated list of the configurations
# to build
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
$CMAKE_TOOLSET = Set-Config-Variable CMAKE_TOOLSET ""
$FEATURE_DOC = Set-Config-Variable FEATURE_DOC "OFF"
$FEATURE_JNI = Set-Config-Variable FEATURE_JNI "OFF"
$FEATURE_PY = Set-Config-Variable FEATURE_PY "OFF"
$FEATURE_LOADTESTS = Set-Config-Variable FEATURE_LOADTESTS "OFF"
$FEATURE_TOOLS = Set-Config-Variable FEATURE_TOOLS "ON"
$FEATURE_TESTS = Set-Config-Variable FEATURE_TESTS "ON"
$PLATFORM = Set-Config-Variable PLATFORM "x64"
$PACKAGE = Set-Config-Variable PACKAGE "NO"
$SUPPORT_SSE = Set-Config-Variable SUPPORT_SSE "ON"
$SUPPORT_OPENCL = Set-Config-Variable SUPPORT_OPENCL "OFF"
$OPENGL_ES_EMULATOR = Set-Config-Variable OPENGL_ES_EMULATOR `
  "c:/Imagination` Technologies/PowerVR_Graphics/PowerVR_Tools/PVRVFrame/Library/Windows_x86_64"
$WIN_CODE_SIGN_IDENTITY = Set-Config-Variable WIN_CODE_SIGN_IDENTITY ""
$WIN_CS_CERT_SEARCH_MACHINE_STORE = Set-Config-Variable WIN_CS_CERT_SEARCH_MACHINE_STORE "OFF"

if ($FEATURE_LOADTESTS -eq "ON")  { $need_gles_emulator=1 }

echo "Configure KTX-Software (Windows $CMAKE_GEN) dir: $BUILD_DIR Arch: $PLATFORM Toolset: $CMAKE_TOOLSET FEATURE_LOADTESTS: $FEATURE_LOADTESTS, FEATURE_DOC: $FEATURE_DOC, FEATURE_JNI: $FEATURE_JNI, FEATURE_PY: $FEATURE_PY, FEATURE_TOOLS: $FEATURE_TOOLS, FEATURE_TESTS: $FEATURE_TESTS, SUPPORT_SSE: $SUPPORT_SSE, SUPPORT_OPENCL: $SUPPORT_OPENCL WIN_CODE_SIGN_IDENTITY: $WIN_CODE_SIGN_IDENTITY WIN_CS_CERT_SEARCH_MACHINE_STORE: $WIN_CS_CERT_SEARCH_MACHINE_STORE"

if (($PACKAGE -eq "YES") -and ($FEATURE_TOOLS -eq "OFF")) {
  echo "Error: Cannot package a configuration that does not build tools. Set FEATURE_TOOLS to ON or PACKAGE to NO"
  exit 2
}

$TOOLSET_OPTION = ""
if($CMAKE_TOOLSET){
  $TOOLSET_OPTION = "-T $CMAKE_TOOLSET"
}

cmake . -G "$CMAKE_GEN" -A $PLATFORM $TOOLSET_OPTION -B $BUILD_DIR `
  -D KTX_FEATURE_DOC=$FEATURE_DOC `
  -D KTX_FEATURE_JNI=$FEATURE_JNI `
  -D KTX_FEATURE_PY=$FEATURE_PY `
  -D KTX_FEATURE_LOADTEST_APPS=$FEATURE_LOADTESTS `
  -D KTX_FEATURE_TOOLS=$FEATURE_TOOLS `
  -D KTX_FEATURE_TESTS=$FEATURE_TESTS `
  -D BASISU_SUPPORT_SSE=$SUPPORT_SSE `
  -D BASISU_SUPPORT_OPENCL=$SUPPORT_OPENCL `
  -D WIN_CODE_SIGN_IDENTITY=$WIN_CODE_SIGN_IDENTITY `
  -D WIN_CS_CERT_SEARCH_MACHINE_STORE=$WIN_CS_CERT_SEARCH_MACHINE_STORE `
  $(if ($need_gles_emulator) {"-D"}) $(if ($need_gles_emulator) {"OPENGL_ES_EMULATOR=$OPENGL_ES_EMULATOR"})

$configArray = $CONFIGURATION.split(",")
foreach ($config in $configArray) {
  pushd $BUILD_DIR
  try {
    #git status
    echo "Build KTX-Software (Windows $PLATFORM $config)"
    cmake --build . --config $config
    # Return an error code if cmake fails
    if(!$?){
      popd
      exit 1
    }

    #git status
    if ($PACKAGE -eq "YES" -and $config -eq "Release") {
    echo "Pack KTX-Software (Windows $PLATFORM $config)"
      cmake --build . --config $config --target PACKAGE
      # Return an error code if cmake fails
      if(!$?){
        popd
        exit 1
      }
    }
    echo "Done building."
  } finally {
   popd
  }
}

