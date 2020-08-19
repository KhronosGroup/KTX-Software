To Do List                     {#todo}
=======

<!--
 Can't put at start. Doxygen requires page title on first line.
 Copyright 2013-2020 Mark Callow 
 SPDX-License-Identifier: Apache-2.0
-->

$Date$

=== **Volunteers welcome!** ===

library
-------

- [x] Find a way so that applications do not have to define KTX_OPENGL{,_ES*} when
      using the library.
- [x] make reader that is usable without OpenGL context
- [ ] use TexStorage in GL texture loader when available
- [x] add Vulkan texture loader

library testing
---------------

- [x] test for GL-context-free reader
- [x] test for Vulkan loader
- [x] proper mipmap test (multiple planes each showing a different miplevel)
- [ ] GLES2 load tests
- [x] cubemap & array texture loading tests for Vulkan
- [ ] 3D texture loading test for Vulkan
- [x] cubemap & array texture loading tests for OpenGL
- [ ]  3D texture loading tests for OpenGL
- [ ] port test framework to Android.

toktx
-----

- [x] support reading formats other than PPM
- [ ] create ddx2ktx tool.
- [x] support 3D and array textures
- [ ] Source & reference ktx files for 1D textures
- [x] Source & reference ktx files for cubemap & array texture creation tests
- [ ] Source & reference ktx files for 3D texture creation tests
