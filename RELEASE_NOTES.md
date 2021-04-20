<!-- Copyright 2021, The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->
Release Notes
=============
## Version 4.0.0
### Significant Changes since Release Candidate 1

* Basis Universal has been updated to version 1.15.

* Errors in both `ktx2check` and `ktxinfo` causing bogus out of memory messages when there is no metadata or an empty value have been fixed.

* An issue in msc\_basis\_transcoder causing intermittent Javascript
"Cannot perform Construct on a detached ArrayBuffer" errors has been
fixed. _NOTE:_ that msc\_basis\_transcoder is deprecated and will be
replaced by the transcoder wrapper from the
[Basis Universal](https://github.com/BinomialLLC/basis_universal)
repository.

### Known Issues in v4.0.0.

* `toktx` will not read JPEG files with a width or height > 32768 pixels.

* `toktx` will not read 4-component JPEG files such as those sometimes
created by Adobe software where the 4th component can be used to re-create
a CMYK image.

* Emscripten versions greater than 2.0.15 have an
[issue](https://github.com/emscripten-core/emscripten/issues/13926)
that causes the Javascript wrapper for libktx to fail. The downloadable
package `KTX-Software-4.0.0-rc1-Web-libktx.zip` has been built with
Emscripten 2.0.15. You only need to be aware of this if building the
wrapper yourself with your own installed emsdk.

* Users making Basisu encoded or block compressed textures for WebGL
must be aware of WebGL restrictions with regard to texture size and
may need to resize images appropriately using the --resize feature
of `toktx`.  In general the dimensions of block compressed textures
must be a multiple of the block size and, if
`WEBGL_compressed_texture_s3tc` on WebGL 1.0 is expected to be one
of the targets, then the dimensions must be a power of 2. For
portability glTF's KHR\_texture\_basisu extension requires texture
dimensions to be a multiple of 4, the block size of the Universal texture
formats.

* Basis Universal encoding results (both ETC1S/LZ and UASTC) are
non-deterministic across platforms. Results are valid but level
sizes and data will differ slightly.
See [issue #60](https://github.com/BinomialLLC/basis_universal/issues/60)
in the basis_universal repository.

* UASTC RDO results differ from run to run unless multi-threading
or RDO multi-threading is disabled. In `toktx` use `--threads 1` for the
former or `--uastc_rdo_m` for the latter. As with the preceeding issue
results are valid but level sizes will differ slightly. See
[issue #151](https://github.com/BinomialLLC/basis_universal/issues/151)
in the basis_universal repository.

### Changes since v4.0.0-rc1 (by part)
### libktx

* Adapt for Basisu 1.15. (a0642fa1) (@MarkCallow)

  * Use zstd included in basisu.
  * Regen reference images with updated ETC1S encoder and newer zstd version.

* git subrepo pull lib/basisu (c7211336) (@MarkCallow)

  subrepo:
    subdir:   "lib/basisu"
    merged:   "5337227c"
  upstream:
    origin:   "https://github.com/BinomialLLC/basis\_universal.git"
    branch:   "master"
    commit:   "5337227c"
  git-subrepo:
    version:  "0.4.3"
    origin:   "https://github.com/MarkCallow/git-subrepo.git"
    commit:   "c1f1132"

* Point .gitsubrepo at correct parent. (02c43d57) (@MarkCallow)

* Minor reformat (#399) (a78c3b46) (@lexaknyazev)

* Handle PVRTC1 minimum 2 block requirement. Fixes issue #390. (#398) (2034ce71) (@MarkCallow)

* Fix: Handle metadata with empty values. (02652303) (@MarkCallow)

  Incidental to the main fix, fix memory leaks in texturetests.

* Fix: properly handle 0 length kvdata. (aee7a1c5) (@MarkCallow)

* Fix error in example. (b7563ea6) (@MarkCallow)

* 2 small fixes: (50000ca6) (@MarkCallow)

  * Raise error in GLUpload on attempted upload of universal texture.
  * In ktx2check don't combine FLOAT & NORM when checking VK\_FORMAT name.

### Tools

* Copy all image attributes when resampling. (83518cdc) (@MarkCallow)

* Skip mipPadding also when no sgd or kvd. Fixes #395. (#396) (fa739a2d) (@MarkCallow)

* 2 small fixes: (50000ca6) (@MarkCallow)

  * Raise error in GLUpload on attempted upload of universal texture.
  * In ktx2check don't combine FLOAT & NORM when checking VK\_FORMAT name.



### JS Wrappers

* Obtain HEAP references after resizing vectors. (5bf11d8f) (@MarkCallow)

  Possible fix for issue #371.


