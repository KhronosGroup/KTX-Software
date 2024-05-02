# Copyright 2022 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

#   Allow setting of variables on the command line. A command line parameter
# must look like (including the quotes shown) '$VAR="string"'. Spaces
# around the equals are acceptable.
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

# These defaults are here to permit easy running of the script locally
# when debugging is needed. Use local variables to avoid polluting the
# environment. Some case have been observed where setting env. var's here
# sets them for the parent as well.
$FEATURE_LOADTESTS = Set-ConfigVariable FEATURE_LOADTESTS "OpenGL+Vulkan"
$FEATURE_TESTS = Set-ConfigVariable FEATURE_TESTS "ON"
$SUPPORT_OPENCL = Set-ConfigVariable SUPPORT_OPENCL "OFF"
$OPENCL_SDK_HOME = Set-ConfigVariable OPENCL_SDK_HOME "https://github.com/intel/llvm/releases/download/2021-09"
$OPENCL_SDK_NAME = Set-ConfigVariable OPENCL_SDK_NAME "win-oclcpuexp-2021.12.9.0.24_rel"
$OPENGL_ES_EMULATOR = Set-ConfigVariable OPENGL_ES_EMULATOR "C:/Imagination/Windows_x86_64"
$OPENGL_ES_EMULATOR_WIN = Set-ConfigVariable OPENGL_ES_EMULATOR_WIN "C:\Imagination\Windows_x86_64"
$PVR_SDK_HOME = Set-ConfigVariable PVR_SDK_HOME "https://github.com/powervr-graphics/Native_SDK/raw/master/lib/Windows_x86_64/"
$VULKAN_SDK_VER = Set-ConfigVariable VULKAN_SDK_VER 1.3.243.0

if ($FEATURE_TESTS -eq "ON") {
  git lfs pull --include=tests/srcimages,tests/testimages
}

if ($FEATURE_LOADTESTS -and $FEATURE_LOADTESTS -ne "OFF") {
  # Must be in repo root for this lfs pull.
  git lfs pull --include=other_lib/win
  if ($FEATURE_LOADTESTS -match "OpenGL") {
    echo "Download PowerVR OpenGL ES Emulator libraries (latest version)."
    $null = md $OPENGL_ES_EMULATOR_WIN
    pushd $OPENGL_ES_EMULATOR_WIN
    # Must use `curl.exe` as `curl` is an alias for the totally different
    # Invoke-WebRequest command which is difficult to use for downloads.
    # curl writes its progress meter to stderr which means PS prints the
    # output with a bright red background so sadly we turn off the meter
    # (-s, --silent) then turn actual error messages back on (-S --show-error).
    curl.exe -s -S -L -O $PVR_SDK_HOME/libGLES_CM.dll
    curl.exe -s -S -L -O $PVR_SDK_HOME/libGLES_CM.lib
    curl.exe -s -S -L -O $PVR_SDK_HOME/libGLESv2.dll
    curl.exe -s -S -L -O $PVR_SDK_HOME/libGLESv2.lib
    curl.exe -s -S -L -O $PVR_SDK_HOME/libEGL.dll
    curl.exe -s -S -L -O $PVR_SDK_HOME/libEGL.lib
    popd
  }
  if ($FEATURE_LOADTESTS -match "Vulkan") {
    echo "Install VulkanSDK."
    pushd $env:TEMP
    curl.exe -s -S -o VulkanSDK-Installer.exe "https://sdk.lunarg.com/sdk/download/$VULKAN_SDK_VER/windows/VulkanSDK-$VULKAN_SDK_VER-Installer.exe?Human=true"
    Start-Process .\VulkanSDK-Installer.exe -ArgumentList "--accept-licenses --default-answer --confirm-command install" -NoNewWindow -Wait
    echo "Return to cloned repo."
    popd
    $key='HKLM:\SYSTEM\CurrentControlSet\Control\Session Manager\Environment'
    $VULKAN_SDK=(Get-ItemProperty -Path $key -Name VULKAN_SDK).VULKAN_SDK
    echo "VULKAN_SDK=$VULKAN_SDK"
  }
}

function Augment-UserPath {
  param ( $PathAddition )
  $key='HKCU:\Environment'
  $curUserPath=(Get-ItemProperty -Path $key -Name Path).Path
}

if ($SUPPORT_OPENCL -eq "ON") {
  # Must be in repo root for this lfs pull.
  git lfs pull --include=lib/basisu/opencl
  echo "Download and install OpenCL CPU runtime..."
  echo "... in sibling of cloned repo (../$OPENCL_SDK_NAME)."
  pushd ..
  echo "curl.exe -s -S -L -O \"$env:OPENCL_SDK_HOME/$env:OPENCL_SDK_NAME.zip\""
  # The quotes prevent some strange variable expansion behavior in PS.
  # The curl command actually works without quotes as foo/$VAR.zip works
  # but play it safe. In the 7z command $VAR.zip results in an empty
  # string. And without the quotes -o$VAR results in "-o$VAR" being
  # passed to the command.
  curl.exe -s -S -L -O "$OPENCL_SDK_HOME/$OPENCL_SDK_NAME.zip"
  echo "7z -o\"$OPENCL_SDK_NAME\" e \"$OPENCL_SDK_NAME.zip\""
  7z -o"$OPENCL_SDK_NAME" e "$OPENCL_SDK_NAME.zip"

  # Can't use $env:Path=$env:Path;... as it won't be seen by caller.
  # Can't use setx $env:Path;... to write it to the registry because
  # setx truncates the target when > 1025 bytes and Path in Appveyor and
  # GitHub Actions is > 1025. Furthermore this ultimately results in the
  # system Path being duplicated as the setx result containing it is
  # written to the user Path.
  Augment-UserPath "$PWD.Path\$OPENCL_SDK_NAME"
  echo "Return to cloned repo."
  popd
}

