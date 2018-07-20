To Do List                     {#todo}
=======

$Date$

=== **Volunteers welcome!** ===

library
-------

- [ ] Find a way so that applications do not have to define KTX_OPENGL{,_ES*} when
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
- [ ] 3D texture test for Vulkan
- [ ] cubemap, 3D & array texture loading tests for OpenGL
- [ ] port test framework to Android.

toktx
-----

- [ ] support reading formats other than PPM, via [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h)
- [ ] create ddx2ktx tool.
- [ ] support 3D and array textures
- [ ] PPM & reference ktx files for 1D textures
- [ ] PPM & reference ktx files for cubemap, 3D & array texture creation tests
