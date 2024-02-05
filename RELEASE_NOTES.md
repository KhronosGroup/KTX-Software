<!-- Copyright 2024, The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->
Release Notes
=============
## Version 4.3.0
### New Features in Version 4.3.0
#### Command Line Tools Suite

v4.3.0 contains a new suite of command line tools accessed via an umbrella `ktx` command.

| Tool | Description | Equivalent old tool |
| :--- | ----------- | ------------------- |
| `ktx create` | Create a KTX2 file from various input files | `toktx` |
| `ktx extract` | Export selected images from a KTX2 file | - |
| `ktx encode` | Encode a KTX2 file to a Basis Universal format | `ktxsc` |
| `ktx transcode` | Transcode a KTX2 file | - |
| `ktx info` | Prints information about a KTX2 file | `ktxinfo` |
| `ktx validate` | Validate a KTX2 file | `ktx2check` |
| `ktx help` | Display help information about the ktx tools | - |

Equivalent old tools will be removed in a subsequent release soon.

Some features of the old tools are not currently available in the new equivalent.

| Old Tool | New Tool | Missing Features |
| :------: | :------: | ---------------- |
| `toktx`  | `create` | JPEG and NBPM input and scaling/resizing of input images. |
| `ktxsc`  | `encode` | ASTC encoding of a KTX2 file. This can be done in `create`. <br>Deflation of a KTX2 file with zlib or zstd.|


The command-line syntax and semantics differ from the old tools including, but not limited to:

* KTX 1.0 files are not supported by the new tools.

* Words in multi-word option names are connected with `-` instead of `_`.
* Individual option names may differ between the old and new tools.
* The `ktx validate` tool may be stricter than `ktx2check` or otherwise differ in behavior, as the new tool enforces all rules of the KTX 2.0 specification. In addition, all new tools that accept KTX 2.0 files as input will be validated in a similar fashion as they would be with the `ktx validate` tool and will fail on the first specification rule violation, if there is one. It also has the option to output the validation results in human readable text format or in JSON format (both formatted and minified options are available), as controlled by the `--format` command-line option.
* The `ktx validate` tool also supports validating KTX 2.0 files against the additional restrictions defined by the _KHR\_texture\_basisu_ extension. Use the `--gltf-basisu` command-line option to verify glTF and WebGL compatibility.
* The new `ktx info` tool produces a unified and complete output of all metadata in KTX 2.0 files and can provide output in human readable text format or in JSON format (both formatted and minified options are available), as controlled by the `--format` command-line option.
* The source repository also includes the JSON schemas that the JSON outputs of the `ktx info` and `ktx validate` tools comply to.
* The `ktx create` tool takes an explicit Vulkan format argument (`--format`) instead of inferring the format based on the provided input files as `toktx`, and thus doesn't perform any implicit color-space conversions except gamma 2.2 to sRGB. Use the `--assign-oetf`, `--convert-oetf`, `--assign-primaries`, and the new `--convert-primaries` for fine grained control over color-space interpretation and conversion.
* The `ktx create` tool does not support resizing or scaling like `toktx`, and, in general, does not perform any image transformations except the optional color-space conversion and mipmap generation options. Users should resize input images to the appropriate resolution before converting them to KTX 2.0 files.
* The `ktx create` and `ktx extract` tools consume and produce, respectively, image file formats that best suit the used Vulkan format. In general, barring special cases, 8-bit and 16-bit normalized integer formats are imported from and exported to PNG files, while integer and floating point formats are imported from and exported to EXR files based on predefined rules. This may be extended in the future using additional command line options and in response to support for other image file formats.
* The new tools and updated `libktx` support ZLIB supercompression besides the BasisLZ and Zstd supercompression schemes supported previously.

Please refer to the manual pages or use the `--help` command-line option for further details on the options available and associated semantics for each individual command.

Thanks to @aqnuep and @VaderY of [RasterGrid](https://www.rastergrid.com/) for their excellent work on the new tool suite.

#### Python Binding
A Python binding for `libktx` has been added. Applications written in Python can now use `libktx` functions. The package can be installed via `pip install pyktx==4.3.0`. Huge thanks to @ShukantPal.

### Changes

* `libktx` has been made much more robust to errors in KTX files.
* `libktx` now validates checksums when present in a Zstd data stream.
* `libktx` has two new error codes it can return: `KTX_DECOMPRESS_LENGTH_ERROR` and `KTX_DECOMPRESS_CHECKSUM_ERROR`.
* All tools and `libktx` now correctly process on all platforms utf8 file names
with multi-byte code-points. Previously such names did not work on Windows.
* The Vulkan texture uploader can now optionally be used with an extenal memory allocator such as [VulkanMemoryAllocator](https://gpuopen.com/vulkan-memory-allocator/).

### Known Issues

* Some image bits in output files encoded to ASTC, ETC1S/Basis-LZ or UASTC on arm64 devices may differ from those encoded from the same input images on x86_64 devices. The differences will not be human visible and will only show up in bit-exact comparisons. 

* Users making Basis Universal encoded or GPU block compressed textures for WebGL must be aware of WebGL restrictions with regard to texture size and may need to resize input images appropriately before using the `ktx create` tool, or use the `--resize` feature of the old `toktx` tool to produce an appropriately sized texture. In general, the dimensions of block compressed textures must be a multiple of the block size in WebGL and for WebGL 1.0 textures must have power-of-two dimensions. Additional portability restrictions apply for glTF per the _KHR\_texture\_basisu_ extension which can be verified using the `--gltf-basisu` command-line option of the new `ktx validate` tool.

* Basis Universal encoding results (both ETC1S/LZ and UASTC) are non-deterministic across platforms. Results are valid but level sizes and data will differ slightly.  See [issue #60](https://github.com/BinomialLLC/basis_universal/issues/60) in the basis_universal repository.

* UASTC RDO results differ from run to run unless multi-threading or RDO multi-threading is disabled. In `toktx` use `--threads 1` for the former or `--uastc_rdo_m` for the latter. As with the preceeding issue results are valid but level sizes will differ slightly. See [issue #151](https://github.com/BinomialLLC/basis_universal/issues/151) in the basis_universal repository.

* Neither the Vulkan nor GL loaders support depth/stencil textures.


### Changes since v4.3.0-beta1 (by part)
### libktx

* Rename branch to match upstream. (aaba10479) (@MarkCallow)

* Add option to enable mkvk targets (VkFormat-related file generation). (#840) (e77a5316f) (@MarkCallow)

### Tools

* Report error on excess filenames (#843) (c32d99a08) (@aqnuep)

* Fix error message typo (#837) (7dedd7e60) (@aqnuep)

* Fix creating 3D textures and add KTXwriterScParams support to transcode (#833) (01d220c36) (@aqnuep)

* Move buffer size check to base class. (dab91cf8e) (@MarkCallow)








## Version 4.3.0-beta1


### Changes since v4.3.0-alpha3 (by part)
### libktx

* git subrepo push lib/dfdutils (ab9c27707) (@MarkCallow)

* Reenable build of loadtest apps on Windows arm64 CI (#802) (6c131f75f) (@MarkCallow)

* Utf-8/unicode support in legacy tools and lib. (#800) (1c5dc9cf6) (@MarkCallow)

* Do not redefine NOMINMAX (#801) (6dbb24643) (@corporateshark)

* libktx: update ktxTexture2\_setImageFromStream to allow setting the entire level's data in one call (#794) (88fc7a6e9) (@AlexRouSg)

* Update dfdutils-included vulkan\_core.h. (#783) (9c223d950) (@MarkCallow)

* Support for A8 and A1B5G5R5 formats (#785) (eeac6206c) (@aqnuep)

* Major non-content documentation fixes. (#773) (e6a6a3be9) (@MarkCallow)

* Fix vendor-specific/tied memory property flag detection (#771) (a10021758) (@toomuchvoltage)

* Return KTX\_NOT\_FOUND when a GPU proc is not found. (#770) (aeca5e695) (@MarkCallow)

* Support for external allocators: (#748) (6856fdb0d) (@toomuchvoltage)

* Fix ktx\_strncasecmp (#741) (1ae04f897) (@VaderY)

* Use correct counter for indexing sample. (#739) (3153e94e8) (@MarkCallow)

### Tools

* Disallow ASTC options when format is not ASTC (#809) (d3ef5ed8b) (@aqnuep)

* Remove unnecessary nullptr checks. (#807) (072a4eb25) (@MarkCallow)

* Set tools and tests rpath on Linux. (#804) (928612a71) (@MarkCallow)

* Utf-8/unicode support in legacy tools and lib. (#800) (1c5dc9cf6) (@MarkCallow)

* Support building of loadtest apps with locally installed dependencies (#799) (84ee59dd2) (@MarkCallow)

* Update dfdutils-included vulkan\_core.h. (#783) (9c223d950) (@MarkCallow)

* Add UTF-8 filename support on Windows to ktxtools (#788) (7b6eab5dc) (@aqnuep)

* Support for A8 and A1B5G5R5 formats (#785) (eeac6206c) (@aqnuep)

* KTXwriterScParams support (#779) (f8691ff05) (@aqnuep)

* Use \-- in doc. to get -- not n-dash in output. (#767) (724790094) (@MarkCallow)

* Document convert\_primaries option in toktx. (#765) (3049f5b5e) (@MarkCallow)

* Do target\_type changes only in toktx (#757) (2cf053c19) (@MarkCallow)

* Add support for fewer components in input files (#755) (adcccf152) (@aqnuep)

* Fix --convert-primaries (#753) (e437ec45f) (@aqnuep)

* Fix legacy app input from pipes on Windows. (#749) (3e7fd0af6) (@MarkCallow)

* Improve output determinism and add internal ktxdiff tool for comparing test outputs (#745) (7f67af7e0) (@VaderY)

* Add KTX\_WERROR config option (#746) (dab32db90) (@MarkCallow)

* Fix incorrect index calculations in image conversions (#735) (682f456de) (@VaderY)

* Color-space and documentation related improvements for ktx create (#732) (8b12216f7) (@VaderY)





### Java Wrapper

* Update dfdutils-included vulkan\_core.h. (#783) (9c223d950) (@MarkCallow)




## Version 4.3.0-alpha3


### Changes since v4.3.0-alpha2 (by part)
### libktx

* Improve documentation (#730) (69b1685a) (@MarkCallow)

  - Rework navigation among the multiple Doxygen projects for much easier use.
  - Rename new ktx tool man pages from `ktxtools\_*` to `ktx\_*`
  - Add `ktx` tool mainpage based on RELEASE\_NOTES info.
  - Make minor formatting fix in `ktx` man page.
  - Update acknowledgements.
  - Remove outdated TODO.md.
  - Add script to do `$Date$` keyword smudging. Use it in CI and reference it from
    README.md to avoid repetition of list of files needing smudging.
  - Add `$Date$` keywords to some docs.
  - Remove `$Date$` and #ident keywords that are no longer needed or used.
  - Document the parts of `khr\_df.h` relevant to the libktx API.

* Implement the extended scope and further improvements for ktxtools (#722) (2189c54e) (@VaderY)

  - tools: Implement stdout support
  - tools: Implement stdin support
  - tools: Implement 4:2:2 support
  - tools: Implement support for 10X6 and 10X4 formats
  - tools: Implement support for B10G11R11\_UFLOAT and E5B9G9R9\_UFLOAT formats
  - tools: Complete support for Depth-Stencil formats
  - tools: Improvements, cleanup and bug fixes
  - extract: Implement fragment URI support
  - extract: Implement 4:2:2 sub-sampling
  - validate: Fix and extend padding byte validation checks
  - cts: Add support for stdin/stdout test cases (including binary IO)
  - cts: Add tests to cover new features and capabilities
  - cts: Extend existing tests to improve coverage
  - cts: Test runner now deletes matching output files (this enables easy packaging of mismatching files)
  - cts: Added a new cli arg to the runner script: --keep-matching-outputs to prevent the deletion of matching output files
  - dfdUtils: Implement 4:2:2 support
  - dfdUtils: Implement support for 10X6 and 10X4 formats
  - imageio: Fix stdin support
  - ktx: Add stringToVkFormat to mkvkformatfiles
  - ktx: Implement 3D BC support (ASTC 3D Blocks)
  - ktx: Implement 4:2:2 support
  - ktx: Complete support for Depth-Stencil formats
  - ktx: Improve interpretDFD
  - ktx: Improvements, cleanup and bug fixes
  - cmake: Add CMake generator expression for output directory on Mac

### Tools

* Improve documentation (#730) (69b1685a) (@MarkCallow)

  - Rework navigation among the multiple Doxygen projects for much easier use.
  - Rename new ktx tool man pages from `ktxtools\_*` to `ktx\_*`
  - Add `ktx` tool mainpage based on RELEASE\_NOTES info.
  - Make minor formatting fix in `ktx` man page.
  - Update acknowledgements.
  - Remove outdated TODO.md.
  - Add script to do `$Date$` keyword smudging. Use it in CI and reference it from
    README.md to avoid repetition of list of files needing smudging.
  - Add `$Date$` keywords to some docs.
  - Remove `$Date$` and #ident keywords that are no longer needed or used.
  - Document the parts of `khr\_df.h` relevant to the libktx API.

* Implement the extended scope and further improvements for ktxtools (#722) (2189c54e) (@VaderY)

  - tools: Implement stdout support
  - tools: Implement stdin support
  - tools: Implement 4:2:2 support
  - tools: Implement support for 10X6 and 10X4 formats
  - tools: Implement support for B10G11R11\_UFLOAT and E5B9G9R9\_UFLOAT formats
  - tools: Complete support for Depth-Stencil formats
  - tools: Improvements, cleanup and bug fixes
  - extract: Implement fragment URI support
  - extract: Implement 4:2:2 sub-sampling
  - validate: Fix and extend padding byte validation checks
  - cts: Add support for stdin/stdout test cases (including binary IO)
  - cts: Add tests to cover new features and capabilities
  - cts: Extend existing tests to improve coverage
  - cts: Test runner now deletes matching output files (this enables easy packaging of mismatching files)
  - cts: Added a new cli arg to the runner script: --keep-matching-outputs to prevent the deletion of matching output files
  - dfdUtils: Implement 4:2:2 support
  - dfdUtils: Implement support for 10X6 and 10X4 formats
  - imageio: Fix stdin support
  - ktx: Add stringToVkFormat to mkvkformatfiles
  - ktx: Implement 3D BC support (ASTC 3D Blocks)
  - ktx: Implement 4:2:2 support
  - ktx: Complete support for Depth-Stencil formats
  - ktx: Improve interpretDFD
  - ktx: Improvements, cleanup and bug fixes
  - cmake: Add CMake generator expression for output directory on Mac



### JS Wrappers

* Improve documentation (#730) (69b1685a) (@MarkCallow)

  - Rework navigation among the multiple Doxygen projects for much easier use.
  - Rename new ktx tool man pages from `ktxtools\_*` to `ktx\_*`
  - Add `ktx` tool mainpage based on RELEASE\_NOTES info.
  - Make minor formatting fix in `ktx` man page.
  - Update acknowledgements.
  - Remove outdated TODO.md.
  - Add script to do `$Date$` keyword smudging. Use it in CI and reference it from
    README.md to avoid repetition of list of files needing smudging.
  - Add `$Date$` keywords to some docs.
  - Remove `$Date$` and #ident keywords that are no longer needed or used.
  - Document the parts of `khr\_df.h` relevant to the libktx API.




## Version 4.3.0-alpha2


### Changes since v4.3.0-alpha1 (by part)
### libktx

* Fix alignment, removes tabs (8e4ee5d5) (@abbaswasim)

* Fix memory leak of input\_image in ktxTexture2\_CompressAstcEx (04bdffe0) (@abbaswasim)


## Version 4.3.0-alpha1

### Changes since v4.2.1 (by part)
### libktx

* Merge ktxtools into main (#714) (a6abf2ff) (@VaderY)

### Tools

* Merge ktxtools into main (#714) (a6abf2ff) (@VaderY)



### JS Wrappers

* Merge ktxtools into main (#714) (a6abf2ff) (@VaderY)

### Java Wrapper

* Merge ktxtools into main (#714) (a6abf2ff) (@VaderY)


