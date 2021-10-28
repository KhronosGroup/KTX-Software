# 3.x series change log

This page summarizes the major functional and performance changes in each
release of the 3.x series.

All performance data on this page is measured on an Intel Core i5-9600K
clocked at 4.2 GHz, running astcenc using AVX2 and 6 threads.

<!-- ---------------------------------------------------------------------- -->
## 3.1

**Status:** July 2021

The 3.1 release is the second release in the 3.x series. This release gives
another performance boost, typically between 5 and 20% faster than the 3.0
release, as well as further incremental improvements to image quality. A number
of build system improvements make astcenc easier and faster to integrate into
other projects as a library, including support for building universal binaries
on macOS. Full change list is shown below.

Reminder for users of the library interface - the API is not designed to be
binary compatible across versions, and this release is not compatible with
earlier releases. Please update and rebuild your client-side code using the
updated `astcenc.h` header.

* **General:**
  * **Feature:** RGB color data now supports `-perceptual` operation. The
    current implementation is simple, weighting color channel errors by their
    contribution to perceived luminance. This mimics the behavior of the human
    visual system, which is most sensitive to green, then red, then blue.
  * **Feature:** Codec supports a new low weight search mode, which is a
    simpler weight assignment for encodings with a low number of weights in the
    weight grid. The weight threshold can be overridden using the new
    `-lowweightmodelimit` command line option.
  * **Feature:** All platform builds now support building a native binary.
    Native binaries automatically select the SIMD level based on the default
    configuration of the compiler in use. Native binaries built on one machine
    may use different SIMD options than native binaries build on another.
  * **Feature:** macOS platform builds now support building universal binaries
    containing both `x86_64` and `arm64` target support.
  * **Feature:** Building the command line can be disabled when using as a
    library in another project. Set `-DCLI=OFF` during the CMake configure
    step.
  * **Feature:** A standalone minimal example of the core codec API usage has
    been added in the `./Utils/Example/` directory.
* **Core API:**
  * **Feature:** Config flag `ASTCENC_FLG_USE_PERCEPTUAL` works for color data.
  * **Feature:** Config option `tune_low_weight_count_limit` added.
  * **Feature:** New heuristic added which prunes dual weight plane searches if
    they are unlikely to help. This heuristic is not user controllable.
  * **Feature:** Image quality has been improved. In general we see significant
    improvements (up to 0.2dB) for high bitrate encodings (4x4, 5x4), and a
    smaller improvement (up to 0.1dB) for lower bitrate encodings.
  * **Bug fix:** Arm "none" SIMD builds could be invariant with other builds.
    This fix has also been back-ported to the 2.x LTS branch.

### Performance:

Key for charts:

* Color = block size (see legend).
* Letter = image format (N = normal map, G = greyscale, L = LDR, H = HDR).

**Relative performance vs 3.0 release:**

![Relative scores 3.1 vs 3.0](./ChangeLogImg/relative-3.0-to-3.1.png)

<!-- ---------------------------------------------------------------------- -->
## 3.0

**Status:** June 2021

The 3.0 release is the first in a series of updates to the compressor that are
making more radical changes than we felt we could make with the 2.x series.
The primary goals of the 3.x series are to keep the image quality ~static or
better compared to the 2.5 release, but continue to improve performance.

Reminder for users of the library interface - the API is not designed to be
binary compatible across versions, and this release is not compatible with
earlier releases. Please update and rebuild your client-side code using the
updated `astcenc.h` header.

* **General:**
  * **Feature:** The code has been significantly cleaned up, with improved
    comments, API documentation, function naming, and variable naming.
* **Core API:**
  * **API Change:** The core APIs for `astcenc_compress_image()` and for
    `astcenc_decompress_image()` now accept swizzle structures by `const`
    pointer, instead of pass-by-value.
  * **API Change:** Calling the `astcenc_compress_reset()` and the
    `astcenc_decompress_reset()` functions between images is no longer required
    if the context was created for use by a single thread.
  * **Feature:** New heuristics have been added for controlling when to search
    beyond 2 partitions and 1 plane, and when to search beyond 3 partitions and
    1 plane. The previous `tune_partition_early_out_limit` config option has
    been removed, and replaced with two new options
    `tune_2_partition_early_out_limit_factor` and
    `tune_3_partition_early_out_limit_factor`. See command line help for more
    detailed documentation.
  * **Feature:** New heuristics have been added for controlling when to use
    dual weight planes. The previous `tune_two_plane_early_out_limit` has been
    renamed to`tune_2_plane_early_out_limit_correlation`. See command line help
    for more detailed documentation.
  * **Feature:** Support for using dual weight planes has been restricted to
    single partition blocks; it rarely helps blocks with 2 or more partitions
    and takes considerable compression search time.

### Performance:

Key for charts:

* Color = block size (see legend).
* Letter = image format (N = normal map, G = greyscale, L = LDR, H = HDR).

**Absolute performance vs 2.5 release:**

![Absolute scores 3.0 vs 2.5](./ChangeLogImg/absolute-2.5-to-3.0.png)

**Relative performance vs 2.5 release:**

![Relative scores 3.0 vs 2.5](./ChangeLogImg/relative-2.5-to-3.0.png)
