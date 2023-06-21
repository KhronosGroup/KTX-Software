<!-- Copyright 2023, The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->
Release Notes
=============
## Version 4.2.1


### Changes since v4.2.0 (by part)
### libktx

* Fix memory leak of input\_image in ktxTexture2\_CompressAstcEx (fa1fe4d7) (@null)










## Version 4.2.0
### Overview

v4.2.0 has few user-facing changes. Most of the work has been behind the scenes improving the build system and fixing warnings across the many supported compilers. User-facing changes are detailed below.

### New Features in v4.2.0

* Install packages for GNU/Linux on Arm64 have been added.

* The Java wrapper is now included in the Windows Arm64 install package.

### Significant Changes since v4.1.0

* The following behavioral changes have been made to `toktx`:

    * If the input PNG file has a gAMA chunk with a value 45460 the image data is now converted to the sRGB transfer function intead of just assigning sRGB as the transfer function of the output file.
    * If the gAMA chunk has a value other than 45640 or 100000 `toktx` will now exit with an error. Previously it used heuristics to decide whether to transform the input to linear or sRGB. Use `--convert_oetf` or `--assign_oetf` to specified the desired behavior.

* The Khronos Data Format header file `KHR/khr_df.h` has been added to the install packages and is included in `ktx.h`. A new transfer function query `ktxTexture2_GetOETF_e` that returns a `khr_df_transfer_e` replaces `ktxTexture2_GetOETF` that returned a `ktx_uint32_t`. The latter is still available for backward compatibility, A new `ktxTexture2_GetColorModel_e` query has been added returning a `khr_df_model_e`.

### Known Issues in v4.2.0.

* Some image bits in output files encoded to ASTC, ETC1S/Basis-LZ or UASTC on arm64 devices may differ from those encoded from the same input images on x86_64 devices. The differences will not be human visible and will only show up in bit-exact comparisons. 

* `toktx` will not read JPEG files with a width or height > 32768 pixels.

* `toktx` will not read 4-component JPEG files such as those sometimes created by Adobe software where the 4th component can be used to re-create a CMYK image.

* Users making Basisu encoded or block compressed textures for WebGL must be aware of WebGL restrictions with regard to texture size and may need to resize images appropriately using the --resize feature of `toktx`.  In general the dimensions of block compressed textures must be a multiple of the block size and for WebGL 1.0 must be a power of 2. For portability glTF's _KHR\_texture\_basisu_ extension requires texture dimensions to be a multiple of 4, the block size of the Universal texture formats.

* Basis Universal encoding results (both ETC1S/LZ and UASTC) are non-deterministic across platforms. Results are valid but level sizes and data will differ slightly.  See [issue #60](https://github.com/BinomialLLC/basis_universal/issues/60) in the basis_universal repository.

* UASTC RDO results differ from run to run unless multi-threading or RDO multi-threading is disabled. In `toktx` use `--threads 1` for the former or `--uastc_rdo_m` for the latter. As with the preceeding issue results are valid but level sizes will differ slightly. See [issue #151](https://github.com/BinomialLLC/basis_universal/issues/151) in the basis_universal repository.

* Neither the Vulkan nor GL loaders support depth/stencil textures.

### Notice

* Building with Visual Studio 2015 and 2017 is no longer supported.

### Changes since v4.1.0 (by part)
### libktx

* Pull upstream ASTC encoder for FP option setting fixes. (#713) (8e68fe04) (@MarkCallow)

* Pull upstream ASTC for fixes building with GCC 11 for arm64, (#700) (514051ca) (@MarkCallow)

* Update Vulkan SDK for macOS CI. (#688) (f57dc8fa) (@MarkCallow)

* CI and Build Improvements (#687) (38f48586) (@MarkCallow)

### Tools

* Pull upstream ASTC for fixes building with GCC 11 for arm64, (#700) (514051ca) (@MarkCallow)

* Reimplement image input handling for toktx. (#702) (1646c4d0) (@MarkCallow)

* Fix normalization when the result overflows (#701) (f81330b5) (@wasimabbas-arm)





### Java Wrapper

* Fix outdated references to `master`. (e724f180) (@MarkCallow)

* Miscellaneous CI script and build fixes (#692) (fefd4a65) (@MarkCallow)

* Remove pinned buffer list in JNI wrapper to avoid segmentation faults (#697) (9b084d50) (@ShukantPal)


