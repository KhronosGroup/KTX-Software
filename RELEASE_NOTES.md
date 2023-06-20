<!-- Copyright 2023, The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->
Release Notes
=============
## Version 4.3.0-alpha2


### Changes since v4.3.0-alpha1 (by part)
### libktx

* Fix alignment, removes tabs (8e4ee5d5) (@abbaswasim)

* Fix memory leak of input\_image in ktxTexture2\_CompressAstcEx (04bdffe0) (@abbaswasim)










## Version 4.3.0-alpha1
### New Features

v4.3.0 contains a new suite of command line tools accessed via an umbrella `ktx` command.

| Tool | Description | Equivalent old tool |
| :--- | ----------- | ------------------- |
| `ktx create` | Create a KTX2 file from various input files | `toktx` |
| `ktx extract` | Export selected images from a KTX2 file | - |
| `ktx encode` | Encode a KTX2 file | `ktxsc` |
| `ktx transcode` | Transcode a KTX2 file | - |
| `ktx info` | Prints information about a KTX2 file | `ktxinfo` |
| `ktx validate` | Validate a KTX2 file | `ktx2check` |
| `ktx help` | Display help information about the ktx tools | - |

Equivalent old tools will be removed in the next release.

Some features of the old tools are not currently available in the new equivalent.

| Old Tool | New Tool | Missing Features |
| :------: | :------: | ---------------- |
| `toktx`  | `create` | JPEG and NBPM input and scaling/resizing. |
| `ktxsc`  | `encode` | ASTC encoding. This can be done in `create`. |

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

### Changes

* `libktx` has been made much more robust to errors KTX files.

* `libktx` now validates checksums when present in a Zstd data stream.
* `libktx` has two new error codes it can return: `KTX_DECOMPRESS_LENGTH_ERROR` and `KTX_DECOMPRESS_CHECKSUM_ERROR`.

### Known Issues

* Some image bits in output files encoded to ASTC, ETC1S/Basis-LZ or UASTC on arm64 devices may differ from those encoded from the same input images on x86_64 devices. The differences will not be human visible and will only show up in bit-exact comparisons. 

* Users making Basis Universal encoded or GPU block compressed textures for WebGL must be aware of WebGL restrictions with regard to texture size and may need to resize input images appropriately before using the `ktx create` tool, or use the `--resize` feature of the old `toktx` tool to produce an appropriately sized texture. In general, the dimensions of block compressed textures must be a multiple of the block size in WebGL and for WebGL 1.0 textures must have power-of-two dimensions. Additional portability restrictions apply for glTF per the _KHR\_texture\_basisu_ extension which can be verified using the `--gltf-basisu` command-line option of the new `ktx validate` tool.

* Basis Universal encoding results (both ETC1S/LZ and UASTC) are non-deterministic across platforms. Results are valid but level sizes and data will differ slightly.  See [issue #60](https://github.com/BinomialLLC/basis_universal/issues/60) in the basis_universal repository.

* UASTC RDO results differ from run to run unless multi-threading or RDO multi-threading is disabled. In `toktx` use `--threads 1` for the former or `--uastc_rdo_m` for the latter. As with the preceeding issue results are valid but level sizes will differ slightly. See [issue #151](https://github.com/BinomialLLC/basis_universal/issues/151) in the basis_universal repository.

* Neither the Vulkan nor GL loaders support depth/stencil textures.


### Changes since v4.2.0 (by part)
### libktx

* Merge ktxtools into main (#714) (a6abf2ff) (@VaderY)

### Tools

* Merge ktxtools into main (#714) (a6abf2ff) (@VaderY)



### JS Wrappers

* Merge ktxtools into main (#714) (a6abf2ff) (@VaderY)

### Java Wrapper

* Merge ktxtools into main (#714) (a6abf2ff) (@VaderY)


