# 5.x series change log

This page summarizes the major functional and performance changes in each
release of the 5.x series.

All performance data on this page is measured on an Intel Core i5-9600K
clocked at 4.2 GHz, running `astcenc` using AVX2 and 6 threads.

<!-- ---------------------------------------------------------------------- -->
## 5.3.0

**Status:** March 2025

The 5.3.0 release is a minor maintenance release.

* **General:**
  * **Feature:** Reference C builds (`ASTCENC_ISA_NONE`) now support compiling
    for big-endian CPUs. Compile with `-DASTCENC_BIG_ENDIAN=ON` when compiling
    for a big-endian target; it is not auto-detected.
  * **Improvement:** Builds using GCC now specify `-flto=auto` to allow
    parallel link steps, and remove the log warnings about not setting a CPU
    count parameter value.
  * **Bug fix:** Builds using MSVC `cl.exe` that do not specify an explicit
    ISA using the preprocessor configuration defines will now correctly
    default to the SSE2 backend on x86-64 and the NEON backend on Arm64. Previously they would have defaulted to the reference C implementation,
    which is around 3.25 times slower.


<!-- ---------------------------------------------------------------------- -->
## 5.2.0

**Status:** February 2025

The 5.2.0 release is a minor maintenance release.

This release includes changes to the public interface in the `astcenc.h`
header.  We always recommend rebuilding your client-side code using the
header from the same release to avoid compatibility issues.

* **General:**
  * **Change:** Changed sRGB alpha channel endpoint expansion to match the
    revised Khronos Data Format Specification (v1.4.0), which reverts an
    unintended specification change. Compared to previous releases, this change
    can cause LSB bit differences in the alpha channel of compressed images.
  * **Feature:** Arm64 builds for Linux added to the GitHub Actions builds, and
    Arm64 binaries for NEON, 128-bit SVE 128 and 256-bit SVE added to release
    builds.
  * **Feature:** Added a new codec API, `astcenc_compress_cancel()`, which can
    be used to cancel an in-flight compression. This is designed to help make
    it easier to integrate the codec into an interactive user interface that
    can respond to user events with low latency.
  * **Bug fix:** Removed incorrect `static` variable qualifier, which could
    result in an incorrect `tune_mse_overshoot` heuristic threshold being used
    if a user ran multiple concurrent compressions with different settings.

<!-- ---------------------------------------------------------------------- -->
## 5.1.0

**Status:** November 2024

The 5.1.0 release is an optimization release, giving moderate performance
improvements on all platforms. There are no image quality differences.

* **General:**
  * **Feature:** Added a new CMake build option to control use of native
    gathers, as they can be slower than scalar loads on some common x86
    microarchitectures. Build with `-DASTCENC_X86_GATHERS=OFF` to disable use
    of native gathers in AVX2 builds.
  * **Optimization:** Added new `gather()` abstraction for gathers using byte
    indices, allowing implementations without gather hardware to skip the
    byte-to-int index conversion.
  * **Optimization:** Optimized `compute_lowest_and_highest_weight()` to
    pre-compute min/max outside of the main loop.
  * **Optimization:** Added improved intrinsics sequence for SSE and AVX2
    integer `hmin()` and `hmax()`.
  * **Optimization:** Added improved intrinsics sequence for `vint4(uint8_t*)`
    on systems implementing Arm SVE.

<!-- ---------------------------------------------------------------------- -->
## 5.0.0

**Status:** November 2024

The 5.0.0 release is the first stable release in the 5.x series. The main new
feature is support for the Arm Scalable Vector Extensions (SVE) SIMD instruction
set.

* **General:**
  * **Bug fix:** Fixed incorrect return type in "None" vector library
    reference implementation.
  * **Bug fix:** Fixed sincos table index under/overflow.
  * **Feature:** Changed `ASTCENC_ISA_NATIVE` builds to use `-march=native` and
    `-mcpu=native`.
  * **Feature:** Added backend for Arm SVE fixed-width 256-bit builds. These
    can only run on hardware implementing 256-bit SVE.
  * **Feature:** Added backend for Arm SVE 128-bit builds. These are portable
    builds and can run on hardware implementing any SVE vector length, but the
    explicit SVE use is augmented NEON and will only use the bottom 128-bits of
    each SVE vector.
  * **Feature:** Optimized NEON mask `any()` and `all()` functions.
  * **Feature:** Migrated build and test to GitHub Actions pipelines.

- - -

_Copyright © 2022-2025, Arm Limited and contributors. All rights reserved._
