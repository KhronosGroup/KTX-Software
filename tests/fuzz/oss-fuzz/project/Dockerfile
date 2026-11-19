# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
################################################################################

FROM gcr.io/oss-fuzz-base/base-builder

# The base image has Clang and the fuzzing engines. The fuzz targets also
# need CMake and Ninja, and Git LFS for the seed files. libktx needs CMake
# 3.23 or later; 3.28 is asked for here. CMake 4 refuses some of the bundled
# third-party projects, hence < 4.
RUN apt-get update && apt-get install -y git-lfs zip && \
    python3 -m pip install "cmake>=3.28,<4" ninja

# The tests/resources files are in Git LFS. Only the KTX and KTX2 files the
# seeds are made from are fetched.
RUN GIT_LFS_SKIP_SMUDGE=1 git clone --depth 1 \
        https://github.com/KhronosGroup/KTX-Software ktx && \
    cd ktx && git lfs install --local && \
    git lfs pull --include="tests/resources/ktx,tests/resources/ktx2"

WORKDIR ktx
COPY build.sh $SRC/
