<!-- Copyright 2021, The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->
Release Notes
=============
## Version 4.0.0-rc1
### Significant Changes

`toktx's` Handling of 1- or 2-component input files has been changed. In accordance with the PNG and JPEG specifications describing files as luminance, it now creates luminance textures.

The following options have been added to `toktx`:

* `--assign_oetf` lets you override the oetf from the input file.
* `--convert_oetf` lets you convert the input image to a different oetf.
* `--input_swizzle` lets you specify a component swizzling to be applied to the input image before it is used to create a KTX file.
* `--swizzle` lets you specify swizzle metadata to be written to a created KTX v2 file.
* `--target_type` lets you modify the number of components of the input image or override the default handling of 1- or 2-component textures.

See the [`toktx` man page](https://github.khronos.org/KTX-Software/ktxtools/toktx.html) for details.

### Changes since v4.0.0-beta8 (by part)
### libktx

* Make luminance{,\_alpha} default for greyscale{,-alpha} input images. Add new features. (#387) (2ffdc81a) (@MarkCallow)

  New features include:
  
      --input\_swizzle & --swizzle
      --target\_type
      --assign\_oetf
      --convert\_oetf
  
  The PR also includes documentation fixes for ktxTexture2\_CompressBasisEx.
  

* git subrepo pull lib/dfdutils (7afa86a5) (@MarkCallow)

  subrepo:
    subdir:   "lib/dfdutils"
    merged:   "659a739b"
  upstream:
    origin:   "https://github.com/KhronosGroup/dfdutils.git"
    branch:   "master"
    commit:   "659a739b"
  git-subrepo:
    version:  "0.4.3"
    origin:   "https://github.com/MarkCallow/git-subrepo.git"
    commit:   "c1f1132"

* git subrepo pull lib/dfdutils (f5310bdd) (@MarkCallow)

  subrepo:
    subdir:   "lib/dfdutils"
    merged:   "c95d443a"
  upstream:
    origin:   "https://github.com/KhronosGroup/dfdutils.git"
    branch:   "master"
    commit:   "c95d443a"
  git-subrepo:
    version:  "0.4.3"
    origin:   "https://github.com/MarkCallow/git-subrepo.git"
    commit:   "c1f1132"

* Fix commit of last pull. (9ccf88fd) (@MarkCallow)

* git subrepo clone https://github.com/KhronosGroup/dfdutils.git lib/dfdutils (22d09c26) (@MarkCallow)

  subrepo:
    subdir:   "lib/dfdutils"
    merged:   "bf8b9961"
  upstream:
    origin:   "https://github.com/KhronosGroup/dfdutils.git"
    branch:   "master"
    commit:   "bf8b9961"
  git-subrepo:
    version:  "0.4.3"
    origin:   "https://github.com/MarkCallow/git-subrepo.git"
    commit:   "c1f1132"

* Remove git subrepo clone of dfdutils ktxsw branch. (0101d6d4) (@MarkCallow)

### Tools

* Make luminance{,\_alpha} default for greyscale{,-alpha} input images. Add new features. (#387) (2ffdc81a) (@MarkCallow)

  New features include:
  
      --input\_swizzle & --swizzle
      --target\_type
      --assign\_oetf
      --convert\_oetf
  
  The PR also includes documentation fixes for ktxTexture2\_CompressBasisEx.
  






