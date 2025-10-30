#! /usr/bin/env bash

# Copyright 2025 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Used only for checking in workflows that the output directory of a
# libktx-only build has only the expected output files.

function usage() {
  echo "$0 <directory> where <directory> is the directory to check."
}

cd $1
# Expect to be in the output of a build created by a multi-config generator.
version=$(cat ../ktx.version)
if [[ -z "$version" ]]; then
  # No file. Perhaps a single config generator was used.
  version=$(cat ktx.version)
fi
if [[ -z "$version" ]]; then
  echo "ktx.version file is missing from $1/.. and $1."
fi
declare -a expected
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
  expected=(libktx.so.$version libktx.so.4 libktx.so libktx_read.so.$version libktx_read.so.4 libktx_read.so)
elif [[ "$OSTYPE" == "darwin"* ]]; then
  expected=(libktx.$version.dylib libktx.4.dylib libktx.dylib libktx_read.$version.dylib libktx_read.4.dylib libktx_read.dylib)
elif [[ "$OSTYPE" == "win32" ]]; then
  expected=(ktx.dll ktx.lib ktx_read.dll ktx_read.lib)
fi
for e in ${expected[@]}; do
  if [[ ! -e $e ]]; then
    echo "Expected file $e does not exist"
    ls
    exit 1
  fi
done

for f in *; do
  expectedFile=0
  for e in ${expected[@]}; do
    if [[ "$e" = "$f" ]] ; then
      expectedFile=1
    fi
  done
  if [[ ! expectedFile -eq 1 ]]; then
      echo "Unexpected file $f found in output directory"
      ls
      exit 1
  fi
done

