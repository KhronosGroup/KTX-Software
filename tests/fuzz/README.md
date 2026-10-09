<!-- Copyright 2026 The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

libFuzzer targets for libktx
============================

`ktx_read_fuzzer` loads each input the way an application loads an untrusted
file: `ktxTexture_CreateFromMemory`, `ktxTexture_LoadImageData`, then for KTX2
`ktxTexture2_TranscodeBasis` when the data needs it, reading every byte of the
image data libktx reports and iterating the levels of data that is not
supercompressed. Textures that declare more than 64 MiB of image data, stored
or inflated, or more than 16 Mi texels are skipped before their data is loaded,
so that huge headers do not end the run with out-of-memory reports.

The targets are a separate CMake project that adds `lib/` as a sub-project,
like `tests/use-ktx`. libktx is built with `-fsanitize=fuzzer-no-link` and the
sanitizers, so libFuzzer gets coverage feedback from libktx itself.

Building
--------

libFuzzer is part of Clang, so a Clang toolchain is required.

```sh
cmake -S tests/fuzz -B build/fuzz -G Ninja \
    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build/fuzz --target ktx_read_fuzzer
```

The default build type is `RelWithDebInfo`, so libktx's `assert()`s are
compiled out. `KTX_FUZZ_SANITIZERS` sets the sanitizers (default
`address,undefined`).

Running
-------

Seed the corpus with small files from `tests/resources`. They are in Git LFS:

```sh
git lfs pull --include=tests/resources/ktx,tests/resources/ktx2
mkdir -p build/fuzz/corpus build/fuzz/seeds
find tests/resources/ktx tests/resources/ktx2 -name '*.ktx*' -size -65k \
    -exec cp {} build/fuzz/seeds/ \;
build/fuzz/ktx_read_fuzzer -dict=tests/fuzz/ktx.dict -max_len=65536 \
    -max_total_time=600 -timeout=30 -rss_limit_mb=2560 \
    build/fuzz/corpus build/fuzz/seeds
```

The first corpus directory receives new inputs; the seeds are only read. A
finding is written to the current directory as `crash-<sha1>`, `leak-<sha1>`,
`timeout-<sha1>` or `oom-<sha1>` (or under `-artifact_prefix=<dir>/`).
Reproduce one by passing it as the only argument:

```sh
build/fuzz/ktx_read_fuzzer crash-<sha1>
```

Continuous integration
----------------------

The `.github/workflows/fuzz.yml` workflow builds the target, fuzzes for 10
minutes from the seeds and uploads any finding as a workflow artifact. It is
separate from the ASan job in `linux.yml` because libFuzzer needs Clang and a
build instrumented for it, while the ASan job builds with GCC and runs the test
suite.
