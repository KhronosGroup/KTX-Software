# Copyright 2023 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Smudge all files with proper $Date$ values.

# Make sure we're in the repo root
$path=(split-path $MyInvocation.MyCommand.Path -Parent)
echo $path
cd $path/..

$target_files = @(
    'pkgdoc/pages.md'
    'lib/libktx_mainpage.md'
    'tools/ktxtools_mainpage.md'
    'interface/js_binding/ktx_wrapper.cpp'
)

#foreach ($file in $target_files) {
#  rm $file
#  git checkout $file
#}
rm $target_files
git checkout  $target_files

