<!-- Copyright 2025 Mark Callow -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# LodePNG

Sourced from https://github.com/KhronosGroup/lodepng/tree/split_decode_inflate
which is a branch in a fork of https://github.com/lvandeve/lodepng.

The branch in the fork has been modified to split decode and data inflation to
allow it to work with our imageio library. A PR has been submitted to merge
these changes upstream: https://github.com/lvandeve/lodepng/pull/206.

The version here has different comments than that in [KhronosGroup/lodepng](https://github.com/KhronosGroup/lodepng/tree/split_decode_inflate).
The one in [KhronosGroup/lodepng](https://github.com/KhronosGroup/lodepng/tree/split_decode_inflate) has been prepared as source for the PR while this one,
in compliance with the license requirements, has comments clearly indicating it has been modified, why and where. The code is identical.

The repo is 3MB but only the 2 files here are needed and it has no tags so 
using `git submodule` or `git subrepo` are unattractive options.


