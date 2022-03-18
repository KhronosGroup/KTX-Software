<!-- Copyright 2019-2020 The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

Library Binary Information for macOS
====================================

Binaries are provided for the following as neither MacPorts or Brew support 
universal binaries or binaries for different architectures installed on the same
machine. Only Release versions of the first 2 are provided.

- `libassimp.a` is a universal binary build of v5.2.2 (with small fixes
  needed to avoid build erors) that includes only the importers. Source URL is
  https://github.com/MarkCallow/assimp.git, a fork of the main assimp repo.
- `libminizip.a` is a universal binary build of
  https://github.com/domoticz/minizip.git.
- `libSDL.dylib` is a universal binary build of tag `Release-2.0.20` from
  https://github.com/libsdl-org/SDL.git.
