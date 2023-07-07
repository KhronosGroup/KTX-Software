@mainpage

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
- The unified KTX tools fronted by `ktx` for creating KTX files from PNG or
  EXR files, validating, encoding and transcoding KTX files, extracting
  images from them and more.
- Legacy tools including `toktx` for creating KTX files from PNG or Netpbm
  format images.

For information about the KTX format see the
<a href="https://www.khronos.org/registry/KTX/">
KTX Registry</a> where you can find the formal specifications
and helpful tools for implementers.

The software is open source software. See @ref license for details.

#### Note on Navigating the Documentation

This GUI provides a unified way to access the four separate KTX document
projects. Due to lack of support in Doxygen for navigating such a collection,
there are some rough edges including, but not limited, to:

* The tab ordering changes. A varying number of tabs related to the currently
  open project are displayed on the left. Tabs for accessing other projects
  follow to the right.
* In the left  pane _treeview_ the entries for accessing other projects have
  the same indentation under the project's title as all the current project's
  pages.

---
@par This page last modified $Date$

@page authors Authors

libKTX is the work of Mark Callow based on work by Georg Kolling and Jacob
Ström with contributions borrowed from Troy Hanson, Johannes van Waveren,
Lode Vandevenne and Rich Geldreich. ASTC encoding was added by Wasim Abbas
of ARM. zlib supercompression, HDR, 422 and depth/stencil format support
were added by Mátyás Császár and Daniel Rákos of RasterGrid who also greatly
improved the robustness of the library.

The libKTX tests are also the work of Mark Callow with some contributions
borrowed from Sascha Willems' Vulkan examples and use Sam Lantinga's libSDL
for portability.

`ktx` and all its commands and the CTS for it were developed by Mátyás Császár
and Daniel Rákos of RasterGrid under contract from The Khronos Group, Inc.

The CLI for `ktx` and its commands was designed by Alexey Knyazev.

`ktx2check`, `ktx2ktx2`, `ktxinfo`, `ktxsc` and `toktx` are the work of
Mark Callow.

The KTX application and file icons were designed by Dominic Agoro-Ombaka.

---
@par This page last modified $Date$

