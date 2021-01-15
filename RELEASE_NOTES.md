<!-- Copyright 2021, The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->
Release Notes
=============
## Version 4.0.0-beta6
### New Features

### Notable Changes

### Known Issues

* Users making Basisu encoded or block compressed textures for WebGL
must be aware of WebGL restrictions with regard to texture size and
may need to resize images appropriately using the --resize feature
of `toktx`.  In general the dimensions of block compressed textures
must be a multiple of the block size and, if
`WEBGL_compressed_texture_s3tc` on WebGL 1.0 is expected to be one
of the targets, then the dimensions must be a power of 2.

* Basis Universal encoding results (both ETC1S/LZ and UASTC) are
non-deterministic across platforms. Results are valid but level
sizes and data will differ slightly.
See [issue #60](https://github.com/BinomialLLC/basis_universal/issues/60)
in the basis_universal repository.

* UASTC RDO results differ from run to run unless multi-threading
is disabled. As with the preceeding issue results are valid but
level sizes will differ slightly.
See [issue #151](https://github.com/BinomialLLC/basis_universal/issues/151)
in the basis_universal repository.

### Changes since v4.0.0-beta5 (by part)
### libktx

* Fix handling of E5R9G9B9 format in initFromDFD. (#353) (5d19bb83) (@MarkCallow)

  Fixes #343.

### Tools

* Fix an issue with the generated KtxTargets.cmake  (#325) (9131fba1) (@UX3D-becher)

  Make it possible to use the find\_package functionality of cmake to use libktx as imported target.






