<!-- Copyright 2024, The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

vcpkg Triplets
==============

These are necessary to get vcpkg to build the dependencies with the same
deployment target as the KTX applications and library. When changing either
deployment target in `../CMakeLists.txt` you must make the same change to the
`*ios*` or `*oxs*` files here.

See https://github.com/microsoft/vcpkg/issues/39981.
