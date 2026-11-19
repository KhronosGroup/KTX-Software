#!/bin/bash -eu
# Copyright 2026 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Builds the fuzz targets for OSS-Fuzz. OSS-Fuzz's own build.sh for the
# project runs this script, so the build can change with the targets
# without a change to OSS-Fuzz. See README.md in this directory.
#
# OSS-Fuzz sets CC, CXX, CFLAGS, CXXFLAGS, LIB_FUZZING_ENGINE, SRC, WORK
# and OUT. CMake reads CC, CXX, CFLAGS and CXXFLAGS from the environment.

ktx=$(cd "$(dirname "$0")/../../.." && pwd)
fuzz=$ktx/tests/fuzz
build=$WORK/build

cmake -S "$fuzz" -B "$build" -G Ninja \
    -DKTX_FUZZ_ENGINE="$LIB_FUZZING_ENGINE"
cmake --build "$build" --parallel "$(nproc)"

# The seeds, as in fuzz.yml: the KTX and KTX2 files of the test resources
# up to 64 KiB, the maximum input length set in the .options files below.
seeds=$WORK/seeds.zip
rm -f "$seeds"
(cd "$ktx/tests/resources" &&
    find ktx ktx2 -name '*.ktx*' -size -65k -print0 | xargs -0 zip -q -j "$seeds")

for target in "$build"/*_fuzzer; do
    [ -f "$target" ] && [ -x "$target" ] || continue
    name=$(basename "$target")
    cp "$target" "$OUT/"
    cp "$seeds" "$OUT/${name}_seed_corpus.zip"
    cp "$fuzz/ktx.dict" "$OUT/$name.dict"
    printf '[libfuzzer]\nmax_len = 65536\n' > "$OUT/$name.options"
done
