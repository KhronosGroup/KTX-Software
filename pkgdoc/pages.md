Abstract        {#abstract}
========

<!--
 Can't put at start. Doxygen requires page title on first line.
 Copyright 2018-2020 The Khronos Groups Inc.
 SPDX-License-Identifier: Apache-2.0
-->

The KTX software consists of
- libktx, a small library of functions for writing and reading KTX (Khronos TeXture)
  files, transcoding those encoded in Basis Universal format and instantiating
  OpenGL<sup>&reg;</sup>, OpenGL ES™️ and
  Vulkan<sup>&reg;</sup> textures from them.
- The KTX tools including `toktx` for creating KTX files from PNG or Netpbm format images.

For information about the KTX format see the
<a href="https://www.khronos.org/registry/KTX/">
KTX Registry</a> where you can find the formal specifications
and helpful tools for implementers.

The software is open source software. See @ref license for details.

---
@par This page last modified $Date$

@page libktx_main libktx

Read the [libktx Reference](libktx/index.html).

Current version is @snippet{doc} version.h Code version
<br>

View the @ref libktx_history

---
@par This page last modified $Date$

@page ktxtools KTX Tools

ktx
----

@ref ktx is a unified front end for the following new tools:

| Tool | Description | Equivalent old tool |
| :--- | ----------- | ------------------- |
| @ref ktx_create | Create a KTX2 file from various input files | `toktx` |
| @ref ktx_extract | Export selected images from a KTX2 file | - |
| @ref ktx_encode | Encode a KTX2 file | `ktxsc` |
| @ref ktx_transcode | Transcode a KTX2 file | - |
| @ref ktx_info | Prints information about a KTX2 file | `ktxinfo` |
| @ref ktx_validate | Validate a KTX2 file | `ktx2check` |
| @ref ktx_help | Display help information about the ktx tools | - |

Equivalent old tools are deprecated and will be removed in the near future.

Some features of old tools are not currently available in their new equivalent.

| Old Tool | New Tool | Missing Features |
| :------: | :------: | ---------------- |
| @ref toktx  | @ref ktx_create "create" | JPEG and NBPM input and scaling/resizing. |
| @ref ktxsc  | @ref ktx_encode "encode" | ASTC encoding. This can be done in `create`. |

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


ktx2check
---------

- @ref ktx2check reference page.
- @ref ktx2check_history.

ktx2ktx2
--------

 - @ref ktx2ktx2 reference page.
 - @ref ktx2ktx2_history.

ktxinfo
-------

 - @ref ktxinfo reference page.
 - @ref ktxinfo_history.

ktxsc
-----

 - @ref ktxsc reference page.
 - @ref ktxsc_history.

toktx
-----

 - @ref toktx reference page.
 - @ref toktx_history.

---
@par This page last modified $Date$

@page authors Authors

libKTX is the work of Mark Callow based on work by Georg Kolling and Jacob
Ström with contributions borrowed from Troy Hanson, Johannes van Waveren,
Lode Vandevenne and Rich Geldreich. ASTC encoding was added by Wasim Abbas.
zlib supercompression, HDR, 422 and depth/stencil format support were added by  Mátyás Császár and Daniel Rákos of RasterGrid who also greatly improved the robustness of the library.

The libKTX tests are also the work of Mark Callow with some contributions
borrowed from Sascha Willems' Vulkan examples and use Sam Lantinga's libSDL
for portability.

`ktx` and all its commands and the CTS for it were developed by Mátyás Császár and Daniel Rákos of RasterGrid under contract from The Khronos Group, Inc.

The CLI for `ktx` and its commands was designed by Alexey Knyazev.

`ktx2check`, `ktx2ktx2`, `ktxinfo`, `ktxsc` and `toktx` are the work of
Mark Callow.

The KTX application and file icons were designed by Dominic Agoro-Ombaka.

---
@par This page last modified $Date$

