<!-- Copyright 2023 The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# Why Vulkan Header Files Here?

The `vulkan_core.h` included here is custom generated from the Vulkan
Registry's `vk.xml` in order to include the format enumerators for the
ASTC 3d formats. We need to support these formats because there are OpenGL implementations even though, as yet, there are no Vulkan implementations.

See `./build_custom_vulkan_core` for instructions on generating the file.

`vulkan_core.h` is used to generate the switch bodies `dfd2vk.inl` and `vk2dfd.inl`.

## To Do

Generate the switch bodies from `vk.xml` instead.
