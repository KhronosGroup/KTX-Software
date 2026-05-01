#! /usr/bin/env bash

# Copyright 2025 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Used only for checking in workflows that the output directory of a
# libktx-only build has only the expected output files.

function usage() {
  echo "$0 <directory> <version> where <directory> is the directory to check and <version> is the expected library version."
}

if [[ $# -ne 2 ]]; then
  echo "$# is the wrong number of arguments"
  usage
  exit 1
fi

cd $1

fullversion=$2
# If the incoming version is a full (package) version that includes
# a tweak, remove it. Library version reflect the API version so do
# not include a tweak.
if [[ $fullversion =~ (([0-9]+).[0-9]+.[0-9]+)(-.*$)? ]]; then
  major=${BASH_REMATCH[1]}
  version=${BASH_REMATCH[2]}
  #echo "major is $major, version is $version"
else
  echo "version number must have the form major.minor.patch[-tweak]".
  usage
  exit 1
fi

declare -a expected
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
  expected=(libktx.so.$version libktx.so.$major libktx.so libktx_read.so.$version libktx_read.so.$major libktx_read.so)
elif [[ "$OSTYPE" == "darwin"* ]]; then
  expected=(libktx.$version.dylib libktx.$major.dylib libktx.dylib libktx_read.$version.dylib libktx_read.$major.dylib libktx_read.dylib)
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

