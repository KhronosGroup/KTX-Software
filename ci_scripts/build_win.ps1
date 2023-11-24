# Copyright 2022 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

#   Allow setting of variables on the command line. A command line parameter
# must look like (including the quotes shown) '$VAR="string"'. Spaces
# around the equals are acceptable.
#   Since the cmake Visual Studio generator generates multi-config build
# trees, CONFIGURATION can be a comma separated list of the config_displays
# to build
for ($i=0; $i -lt $args.length; $i++)
{
  Invoke-Expression $($args[$i])
}

function Set-ConfigVariable {
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

# Build for the local machine by default.
$defaultArch = $env:processor_architecture.toLower()
if ($defaultArch -eq "amd64") {
  $defaultArch = "x64"
} elseif ($defaultArch -ne "arm64") {
  echo "KTX build for Windows does not support $defaultArch architecture."
  echo "Only amd64 and arm64 are supported."
  exit 1
}

# These defaults are here to permit easy running of the script locally
# when debugging is needed. Use local variables to avoid polluting the
# environment. Some cases have been observed where setting env. var's
# here sets them for the parent as well.
$ARCH = Set-ConfigVariable ARCH $defaultArch
$BUILD_DIR = Set-ConfigVariable BUILD_DIR "build/build-batch-vs2022"
$CONFIGURATION = Set-ConfigVariable CONFIGURATION "Release"
$CMAKE_GEN = Set-ConfigVariable CMAKE_GEN "Visual Studio 17 2022"
$CMAKE_TOOLSET = Set-ConfigVariable CMAKE_TOOLSET ""
$FEATURE_DOC = Set-ConfigVariable FEATURE_DOC "OFF"
$FEATURE_JNI = Set-ConfigVariable FEATURE_JNI "OFF"
if ($ARCH -eq 'x64') {
  $FEATURE_LOADTESTS = Set-ConfigVariable FEATURE_LOADTESTS "OpenGL+Vulkan"
} else {
  $FEATURE_LOADTESTS = Set-ConfigVariable FEATURE_LOADTESTS "OpenGL"
}
$FEATURE_PY = Set-ConfigVariable FEATURE_PY "OFF"
$FEATURE_TESTS = Set-ConfigVariable FEATURE_TESTS "ON"
$FEATURE_TOOLS = Set-ConfigVariable FEATURE_TOOLS "ON"
$FEATURE_TOOLS_CTS = Set-ConfigVariable FEATURE_TOOLS_CTS "ON"
$LOADTESTS_USE_LOCAL_DEPENDENCIES = Set-ConfigVariable LOADTESTS_USE_LOCAL_DEPENDENCIES "OFF"
$PACKAGE = Set-ConfigVariable PACKAGE "NO"
$PYTHON = Set-ConfigVariable PYTHON ""
$SUPPORT_SSE = Set-ConfigVariable SUPPORT_SSE "ON"
$SUPPORT_OPENCL = Set-ConfigVariable SUPPORT_OPENCL "OFF"
$WERROR = Set-ConfigVariable WERROR "OFF"
if ($ARCH -eq 'x64') {
  $OPENGL_ES_EMULATOR = Set-ConfigVariable OPENGL_ES_EMULATOR `
    "c:/Imagination` Technologies/PowerVR_Graphics/PowerVR_Tools/PVRVFrame/Library/Windows_x86_64"
} else {
  $OPENGL_ES_EMULATOR = Set-ConfigVariable OPENGL_ES_EMULATOR ""
}
$CODE_SIGN_KEY_VAULT = Set-ConfigVariable CODE_SIGN_KEY_VAULT ""
$CODE_SIGN_TIMESTAMP_URL = Set-ConfigVariable CODE_SIGN_TIMESTAMP_URL ""
$LOCAL_KEY_VAULT_SIGNING_IDENTITY = Set-ConfigVariable LOCAL_KEY_VAULT_SIGNING_IDENTITY ""
$LOCAL_KEY_VAULT_CERTIFICATE_THUMBPRINT  = Set-ConfigVariable LOCAL_KEY_VAULT_CERTIFICATE_THUMBPRINT ""
$AZURE_KEY_VAULT_URL = Set-ConfigVariable AZURE_KEY_VAULT_URL ""
$AZURE_KEY_VAULT_CERTIFICATE = Set-ConfigVariable AZURE_KEY_VAULT_CERTIFICATE ""
$AZURE_KEY_VAULT_CLIENT_ID = Set-ConfigVariable AZURE_KEY_VAULT_CLIENT_ID ""
$AZURE_KEY_VAULT_CLIENT_SECRET = Set-ConfigVariable AZURE_KEY_VAULT_CLIENT_SECRET ""
$AZURE_KEY_VAULT_TENANT_ID = Set-ConfigVariable AZURE_KEY_VAULT_TENANT_ID ""

if ($FEATURE_LOADTESTS -match 'OpenGL')  { $need_gles_emulator=1 }

if (($PACKAGE -eq "YES") -and ($FEATURE_TOOLS -eq "OFF")) {
  echo "Error: Cannot package a configuration that does not build tools. Set FEATURE_TOOLS to ON or PACKAGE to NO"
  exit 2
}

if (($FEATURE_TOOLS_CTS -eq "ON")) {
  git submodule update --init --recursive tests/cts
}

$cmake_args = @(
  "-G", "$CMAKE_GEN"
  "-A", "$ARCH"
)
if($CMAKE_TOOLSET) {
  $cmake_args += @(
    "-T", "$CMAKE_TOOLSET"
  )
}
$cmake_args += @(
  "-B", "$BUILD_DIR"
  "-D", "KTX_FEATURE_DOC=$FEATURE_DOC"
  "-D", "KTX_FEATURE_JNI=$FEATURE_JNI"
  "-D", "KTX_FEATURE_LOADTEST_APPS=$FEATURE_LOADTESTS"
  "-D", "KTX_FEATURE_PY=$FEATURE_PY"
  "-D", "KTX_FEATURE_TESTS=$FEATURE_TESTS"
  "-D", "KTX_FEATURE_TOOLS=$FEATURE_TOOLS"
  "-D", "KTX_FEATURE_TOOLS_CTS=$FEATURE_TOOLS_CTS"
  "-D", "KTX_LOADTEST_APPS_USE_LOCAL_DEPENDENCIES=$LOADTESTS_USE_LOCAL_DEPENDENCIES"
  "-D", "KTX_WERROR=$WERROR"
  "-D", "BASISU_SUPPORT_SSE=$SUPPORT_SSE"
  "-D", "BASISU_SUPPORT_OPENCL=$SUPPORT_OPENCL"
  "-D", "CODE_SIGN_KEY_VAULT=$CODE_SIGN_KEY_VAULT"
  "-D", "PYTHON=$PYTHON"
)
if ($CODE_SIGN_KEY_VAULT) {
  # To avoid CMake warning, only specify this when actually signing.
  $cmake_args += @(
    "-D", "CODE_SIGN_TIMESTAMP_URL=$CODE_SIGN_TIMESTAMP_URL"
  )
}
if ($CODE_SIGN_KEY_VAULT -eq "Azure") {
  $cmake_args += @(
    "-D", "AZURE_KEY_VAULT_URL=$AZURE_KEY_VAULT_URL"
    "-D", "AZURE_KEY_VAULT_CERTIFICATE=$AZURE_KEY_VAULT_CERTIFICATE"
    "-D", "AZURE_KEY_VAULT_CLIENT_ID=$AZURE_KEY_VAULT_CLIENT_ID"
    "-D", "AZURE_KEY_VAULT_CLIENT_SECRET=$AZURE_KEY_VAULT_CLIENT_SECRET"
    "-D", "AZURE_KEY_VAULT_TENANT_ID=$AZURE_KEY_VAULT_TENANT_ID"
  )
} elseif ($CODE_SIGN_KEY_VAULT) {
  $cmake_args += @(
    "-D", "LOCAL_KEY_VAULT_SIGNING_IDENTITY=$LOCAL_KEY_VAULT_SIGNING_IDENTITY"
    "-D", "LOCAL_KEY_VAULT_CERTIFICATE_THUMBPRINT=$LOCAL_KEY_VAULT_CERTIFICATE_THUMBPRINT"
  )
} `
if ($need_gles_emulator) {
  $cmake_args += @("-D", "OPENGL_ES_EMULATOR=$OPENGL_ES_EMULATOR")
}

$config_display = "Configure KTX-Software: "
foreach ($item in $cmake_args) {
  switch ($item) {
       "-A" { $config_display += 'Arch=' }
       "-B" { $config_display += 'Build Dir=' }
       "-D" { }
       "-G" { $config_display += 'Generator=' }
       "-T" { $config_display += 'Toolset=' }
         "" { }
    default { $config_display += "$item, " }
  }
}
$config_display = $config_display -replace ', $', ''
echo $config_display

cmake . $cmake_args

# Return an error code if cmake config fails.
if(!$?){
  exit 1
}

# Find SDK version and ls it
#if ($FEATURE_LOADTESTS -ne "OFF") {
#  $m = select-string -Pattern "<WindowsTargetPlatformVersion>(?<version>(?<major>[0-9][0-9])[0-9\.]*)</.*" -Path $BUILD_DIR/tests/loadtests/gl3loadtests.vcxproj
#  $sdk_ver = $m.matches[0].groups["version"].value
#  $sdk_major = $m.matches[0].groups["major"].value
#  echo "sdk_ver = $sdk_ver"
#  echo "sdk_major = $sdk_major"
#  ls "C:\Program Files (x86)\Windows Kits\$sdk_major\lib"
#  ls "C:\Program Files (x86)\Windows Kits\$sdk_major\lib\$sdk_ver\um\arm64\glu32.lib" -ErrorAction 'Continue'
#  ls "C:\Program Files (x86)\Windows Kits\$sdk_major\lib\$sdk_ver\um\x64\glu32.lib" -ErrorAction 'Continue'
#}

$configArray = $CONFIGURATION.split(",")
foreach ($config in $configArray) {
  pushd $BUILD_DIR
  try {
    #git status
    echo "Build KTX-Software (Windows $ARCH $config)"
    cmake --build . --config $config
    # Return an error code if cmake fails
    if(!$?){
      popd
      exit 1
    }

    #git status
    if ($PACKAGE -eq "YES" -and $config -eq "Release") {
    echo "Pack KTX-Software (Windows $ARCH $config)"
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

