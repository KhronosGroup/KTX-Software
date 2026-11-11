<!-- Copyright 2026 The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

OSS-Fuzz
========

Files for building the fuzz targets on
[OSS-Fuzz](https://google.github.io/oss-fuzz/).

`project/` is meant to be copied to `projects/ktx/` in
[google/oss-fuzz](https://github.com/google/oss-fuzz). Its `build.sh` runs
`build.sh` in this directory, which builds the `tests/fuzz` targets with the
flags OSS-Fuzz passes and installs a seed corpus, `ktx.dict` and a
`.options` file for each target.

The contacts in `project/project.yaml` need to be filled in before
submitting.

To test locally with Docker, from a clone of google/oss-fuzz with `project/`
copied to `projects/ktx/`:

```sh
python3 infra/helper.py build_image ktx
python3 infra/helper.py build_fuzzers ktx /path/to/KTX-Software
python3 infra/helper.py check_build ktx
```
