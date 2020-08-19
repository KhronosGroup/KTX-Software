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
<a href="http://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/">
formal specification.</a>

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
Lode Vandevenne and Rich Geldreich.

The libKTX tests are also the work of Mark Callow with some contributions
borrowed from Sascha Willems' Vulkan examples and use Sam Lantinga's libSDL
for portability.

`ktx2check`, `ktx2ktx2`, `ktxinfo`, `ktxsc` and `toktx` are the work of
Mark Callow.

The KTX application and file icons were designed by Manmohan Bishnoi.

---
@par This page last modified $Date$

