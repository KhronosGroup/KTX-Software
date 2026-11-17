<!-- Copyright 2026 The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

libFuzzer targets for libktx
============================

There are three targets:

- `ktx_read_fuzzer` loads textures with libktx.
- `ktx_info_fuzzer` prints them with the info printers of libktx.
- `ktx_validate_fuzzer` validates them with the validator of the `ktx` tool.

`ktx_read_fuzzer` loads each input the way an application loads an untrusted
file: `ktxTexture_CreateFromMemory`, `ktxTexture_LoadImageData`, then for KTX2
`ktxTexture2_TranscodeBasis` when the data needs it, reading every byte of the
image data libktx reports and iterating the levels of data that is not
supercompressed. Textures that declare more than 64 MiB of image data, stored
or inflated, or more than 16 Mi texels are skipped before their data is loaded,
so that huge files do not end the run with out-of-memory reports.

`ktx_info_fuzzer` prints each input with `ktxPrintInfoForMemory`, as text,
and `ktxPrintKTX2InfoJSONForMemory`, as JSON, as `ktx info` does. The
printers read the header and the blocks it describes themselves, without the
checks of `ktxTexture_CreateFromMemory`. Their output is discarded.

`ktx_validate_fuzzer` runs `validateMemory` from `tools/ktx/validate.cpp` on
each input. That function does the checks of `ktx validate`, which the other
`ktx` commands also run on their input files. For an input of odd size it also
does the glTF checks of `ktx validate --gltf-basisu`. Only `validate.cpp` and
fmt are built, not the whole tool. The validator ends by loading the texture
with libktx, so inputs that libktx accepts and that declare more than 64 MiB of
image data or 16 Mi texels are skipped, as in `ktx_read_fuzzer`. The validator
converts header fields to the C enums of `ktx.h` before checking them, and any
32-bit value can be in the file, so the target is built without
`-fsanitize=enum`.

The targets are a separate CMake project that adds `lib/` as a sub-project,
like `tests/use-ktx`. libktx is built with `-fsanitize=fuzzer-no-link` and the
sanitizers, so libFuzzer gets coverage feedback from libktx itself.

Building
--------

libFuzzer is part of Clang, so a Clang toolchain is required.

```sh
cmake -S tests/fuzz -B build/fuzz -G Ninja \
    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build/fuzz --target ktx_read_fuzzer ktx_info_fuzzer ktx_validate_fuzzer
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

The other targets run the same way. The first corpus directory receives new
inputs; the seeds are only read. Give each target a corpus directory of its
own: the inputs that reach new code differ between them. A
finding is written to the current directory as `crash-<sha1>`, `leak-<sha1>`,
`timeout-<sha1>` or `oom-<sha1>` (or under `-artifact_prefix=<dir>/`).
Reproduce one by passing it as the only argument:

```sh
UBSAN_OPTIONS=print_stacktrace=1 build/fuzz/ktx_read_fuzzer crash-<sha1>
```

The report gives the stack of the error: without `print_stacktrace=1`
UndefinedBehaviorSanitizer gives only its file and line.

Continuous integration
----------------------

The `.github/workflows/fuzz.yml` workflow builds each target in a job of its
own and fuzzes it for 10 minutes from the seeds. When a target finds
something, the job uploads an artifact named after the target. It holds the
finding with the extension of what it is, `crash-<sha1>.ktx2` or `.ktx`, or
`.bin` for other bytes; the log of the run, whose report gives the stack of the
error; and a `README.txt` that says how to reproduce it. The workflow is
separate from the ASan job in `linux.yml` because libFuzzer needs Clang and a
build instrumented for it, while the ASan job builds with GCC and runs the test
suite.
