<!-- Copyright 2021, The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->
Release Notes
=============
## Version 4.0.0-beta8
### Significant Changes

Incoporates Basis Universal 1.13 bringing the latest ETC1S and UASTC
encodersa.

The ETC1S encoder is 3x faster with no quality loss (_toktx_ option
`--clevel 2`) and 4.5x faster with a very slight quality loss
(`--clevel 1`). Note that the default `compressionLevel` parameter
in the _libktx_ API has been changed from 1 to 2. However the default in _toktx_ remains 1, to match _basisu\_tool_.

The UASTC RDO encoder has greatly improved quality per bit making
lower bitrates more usable. As part of this _toktx's_ `--uastc_rdo_q`
option has been renamed `--uastc_rdo_l` (for lamda) to reflect the new
implementation. The range of values to try has changed too. The
UASTC RDO dictionary size default and minimum allowed size have
changed. There are now options to control the new smooth block
detector and an option to disable RDO multithreading for determinism.
See the [toktx man
page](https://github.khronos.org/KTX-Software/ktxtools/toktx.html) for
details.

### Changes since v4.0.0-beta7 (by part)
### libktx

* git subrepo push lib/dfdutils (5d1acb19) (@null)

* Ignore noSSE when SSE support not compiled in. (5a1f9e6c) (@MarkCallow)

* git subrepo pull lib/basisu (edac0216) (@MarkCallow)

* Restore previous value of sse support. (41e23d02) (@MarkCallow)

* Expose new BU encoder options in libktx & apps. (7d0e9641) (@MarkCallow)

* git subrepo pull lib/basisu (69017842) (@MarkCallow)

* Integrate Basis changes into build. (231e828a) (@MarkCallow)

* git subrepo pull --force lib/basisu (8621a855) (@MarkCallow)

* Remove copies of moved and deleted files. (0bc26125) (@MarkCallow)

* Fix compile warnings passing args to new basisu API. (1bdc1eca) (@MarkCallow)

* Support video in cube maps to match relaxation in KTX spec. (#381) (f6c5f548) (@MarkCallow)

### Tools

* Remove workaround for issue with basisu\_resampler.h. (d74c679f) (@MarkCallow)

* Expose new BU encoder options in libktx & apps. (7d0e9641) (@null)

* Fix MSVC confusion & error over intended iterator. (8a00c9c2) (@null)

* Remove copies of moved and deleted files. (0bc26125) (@null)

* Support video in cube maps to match relaxation in KTX spec. (#381) (f6c5f548) (@null)






