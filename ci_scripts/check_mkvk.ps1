# Copyright 2024 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

<#
  .SYNOPSIS
  Check generation of VkFormat related files.

  .DESCRIPTION
  Regenerates all VkFormat related files and compares them with the
  version in Git. Used to verify correct functioning of the generation
  scripts in CI.

  .INPUTS
  None

  .OUTPUTS
  None
#>

param (
  # Default of $null results in an empty string when not set, so be explicit.
  [string] $BUILD_DIR = ""
  # With positional parameters, BUILD_DIR will be $null if no parameter.
#  [Parameter(Position=0)] [string[]]$BUILD_DIR
)

function Get-ParamValue {
  <#
    .SYNOPSIS
    Get a parameter value.

    .DESCRIPTION
    Returns one of the following in this priority order:

      1. Value set on command line, if any.
      2. Value from same-named environment variable, if any
      3. $DefaultValue param.
  #>
  param ( $ParamName, $DefaultValue )
  $res = get-variable $ParamName -ValueOnly -ErrorAction 'SilentlyContinue'
  if ($res -eq "" -or $res -eq $null) {
    $res = [Environment]::GetEnvironmentVariable($ParamName)
    if ($res -eq $null) {
        $res = $DefaultValue
    }
  } 
  return $res
}

$BUILD_DIR = Get-ParamValue BUILD_DIR "build/checkmkvk"

cmake . -B $BUILD_DIR -D KTX_FEATURE_TESTS=OFF -D KTX_FEATURE_TOOLS=OFF -D KTX_GENERATE_VK_FILES=ON
# Clean first is to ensure all files are generated so everything is tested.
cmake --build $BUILD_DIR --target mkvk --clean-first
rm  $BUILD_DIR -Recurse -Confirm:$false
# Verify no files were modified. Exit with 1, if so.
git diff --quiet HEAD
if (!$?) {
    git status
    git diff
    exit 1
}
