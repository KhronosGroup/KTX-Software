<!-- Copyright 2021, The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->
Release Notes
=============
## Version 4.0.0-beta7
### Significant Changes

* An error that prevented KTX files created from PNG files with tRNS chunks having
alpha channels was fixed.

### Changes since v4.0.0-beta6 (by part)
### libktx

* Properly detect presence of a tRNS chunk. Fixes #356. (#370) (08eed131) (@MarkCallow)

* Fix first INVALID\_OPERATION reason. Fixes #358. (4013b1f0) (@MarkCallow)

* Expose BasisU compressor status\_output via a --verbose opt. (#368) (9f40914f) (@MarkCallow)

* Disable status output from BasisU compressor. (97bdfcaf) (@MarkCallow)

* Update for latest BasisU API & deprecate msc\_basis\_transcoder. (ce766b78) (@MarkCallow)

* git subrepo commit (merge) lib/basisu (c2776ffe) (@MarkCallow)

  subrepo:
    subdir:   "lib/basisu"
    merged:   "61785924"
  upstream:
    origin:   "https://github.com/BinomialLLC/basis\_universal.git"
    branch:   "master"
    commit:   "ef70ddd7"
  git-subrepo:
    version:  "0.4.0"
    origin:   "https://github.com/MarkCallow/git-subrepo.git"
    commit:   "65b6406"

### Tools

* Fix reporting of jpeg decoder errors. (79d3f354) (@MarkCallow)

* Properly detect presence of a tRNS chunk. Fixes #356. (#370) (08eed131) (@MarkCallow)

* Rescale to 8-bits when encoding to UASTC. Fixes #360. (6c792d94) (@MarkCallow)

* Update for latest BasisU API & deprecate msc\_basis\_transcoder. (ce766b78) (@MarkCallow)



### JS Wrappers

* Add BC7\_RGBA. Deprecate other BC7 enums. Fixes #369. (a5a812ec) (@MarkCallow)

* Fix duplicate call in transcode example code. (15731f5f) (@MarkCallow)

* Update for latest BasisU API & deprecate msc\_basis\_transcoder. (ce766b78) (@MarkCallow)


