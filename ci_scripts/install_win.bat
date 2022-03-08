@echo off
rem Copyright 2022 The Khronos Group Inc.
rem SPDX-License-Identifier: Apache-2.0

rem This is a .bat file because PS curl is very different.

setlocal EnableDelayedExpansion
setlocal

rem Allow parameter setting on command line. Parameter must look like,
rem including the quotes shown "VAR=string".
for /f "delims=" %%a in (%*) do call set %%a

if [%FEATURE_LOADTESTS%] == [] set FEATURE_LOADTESTS=ON
if [%SUPPORT_OPENCL%] == [] set SUPPORT_OPENCL=OFF

rem pushd/popd doesn't work in at least some of the appveyor environments.
set curdir=%cd%

rem It seems this version or this shell does not support multiple --includes.
git lfs pull --include=tests/srcimages
git lfs pull --include=tests/testimages

if %FEATURE_LOADTESTS% == ON (
  @echo "Download PowerVR OpenGL ES Emulator libraries (latest version)."
  md %OPENGL_ES_EMULATOR_WIN%
  cd %OPENGL_ES_EMULATOR_WIN%
  curl -L -O %PVR_SDK_HOME%/libGLES_CM.dll
  curl -L -O %PVR_SDK_HOME%/libGLES_CM.lib
  curl -L -O %PVR_SDK_HOME%/libGLESv2.dll
  curl -L -O %PVR_SDK_HOME%/libGLESv2.lib
  curl -L -O %PVR_SDK_HOME%/libEGL.dll
  curl -L -O %PVR_SDK_HOME%/libEGL.lib
  @echo Install VulkanSDK.
  cd C:\
  curl -o VulkanSDK-Installer.exe https://sdk.lunarg.com/sdk/download/%VULKAN_SDK_VER%/windows/VulkanSDK-%VULKAN_SDK_VER%-Installer.exe?Human=true
  .\VulkanSDK-Installer.exe /S
)

if %SUPPORT_OPENCL% == ON (
  @echo "Download and install OpenCL CPU runtime..."
  @echo "... in sibling of cloned repo (%APPVEYOR_BUILD_FOLDER%/../opencl)."
  cd %APPVEYOR_BUILD_FOLDER%\..
  echo curl -L -O %OPENCL_SDK_HOME%/%OPENCL_SDK_NAME%.zip
  curl -L -O %OPENCL_SDK_HOME%/%OPENCL_SDK_NAME%.zip
  7z -o%OPENCL_SDK_NAME% e %OPENCL_SDK_NAME%.zip
  rem 'Can't use set PATH=%PATH%;... as it won't be seen by caller.'
  rem 'Can't use setx %PATH%;... to write it to the registry because'
  rem "setx truncates the target when > 1025 bytes and the PATH in"
  rem "appveyor is > 1025. Furthermore this ultimately results in the"
  rem "system PATH being duplicated as the setx result containing it"
  rem "is written to the user PATH."
  call :add_to_user_path "!cd!\%OPENCL_SDK_NAME%"
)
@echo "Return to cloned repo."
cd %curdir%
rem "Must be in repo root for this."
if %FEATURE_LOADTESTS% == ON (
  git lfs pull --include=other_lib/win
)
if %SUPPORT_OPENCL% == ON (
  git lfs pull --include=lib/basisu/opencl
)
goto :end

rem Subroutine "add_to_user_path" -------------------------------
:add_to_user_path
for /f "skip=2 tokens=*" %%A in ('reg query "HKCU\Environment" /v PATH') do (
   set regstr=%%A

   for /f "tokens=3 delims=|" %%X in ("!regstr:    =^|!") do (
     setx PATH "%%X;%~1"
   )
)
exit /b
rem End Subroutine -----------------------------------------------

:end
