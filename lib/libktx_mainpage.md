@mainpage

<!--
 Can't put at start. Doxygen requires page title on first line.
 Copyright 2019-2020 Mark Callow 
 SPDX-License-Identifier: Apache-2.0
-->

libktx is a small library of functions for creating and reading KTX (Khronos
TeXture) files, version 1 and 2 and instantiating OpenGL&reg; and OpenGL&reg; ES
textures and Vulkan images from them. KTX version 2 files can contain images
supercompressed with _zstd_ or _zlib_. They can also contain images in the Basis
Universal formats. libktx can deflate and inflate zstd and zlib compressed
images, can encode and transcode the Basis Universal formats and can
encode ASTC formats.

For information about the KTX format see the
<a href="https://registry.khronos.org/KTX/specs/2.0/ktxspec.v2.html">
formal specification.</a>

@authors
<a href="http://github.com/MarkCallow">Mark Callow</a>,
formerly at <a href="http://www.hicorp.co.jp">HI Corporation</a>\n
Mátyás Császár and Daniel Rákos, <a href="https://www.rastergrid.com/">RasterGrid</a>\n
Wasim Abbas, <a href="https://www.arm.com/">Arm</a>\n
Andreas Atteneder, Independent\n
Georg Kolling, <a href="http://www.imgtec.com">Imagination Technology</a>\n
Jacob Str&ouml;m, <a href="http://www.ericsson.com">Ericsson AB</a>

@snippet{doc} version.h API version

$Date$

# Usage Overview                              {#overview}

The following `ktxTexture` examples work for both KTX and KTX2 textures. The
texture type is determined from the file contents.

## Reading a KTX file for non-GL and non-Vulkan Use  {#readktx}

~~~~~~~~~~~~~~~~{.c}
#include <ktx.h>

ktxTexture* texture = NULL;
KTX_error_code result;
ktx_size_t offset;
ktx_uint8_t* image;
ktx_uint32_t level, layer, faceSlice;

result = ktxTexture_CreateFromNamedFile("mytex3d.ktx",
                                        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                        &texture);

// Retrieve information about the texture from fields in the ktxTexture
// such as:
ktx_uint32_t numLevels = texture->numLevels;
ktx_uint32_t baseWidth = texture->baseWidth;
ktx_bool_t isArray = texture->isArray;

// Retrieve a pointer to the image for a specific mip level, array layer
// & face or depth slice.
level = 1; layer = 0; faceSlice = 3;
result = ktxTexture_GetImageOffset(texture, level, layer, faceSlice, &offset);
image = ktxTexture_GetData(texture) + offset;
// ...
// Do something with the texture image.
// ...
ktxTexture_Destroy(texture);
~~~~~~~~~~~~~~~~

## Creating a GL texture object from a KTX file.   {#createGL}

~~~~~~~~~~~~~~~~{.c}
#include <ktx.h>

ktxTexture* kTexture = NULL;
KTX_error_code result;
ktx_size_t offset;
ktx_uint8_t* image;
ktx_uint32_t level, layer, faceSlice;
GLuint texture = 0;
GLenum target, glerror;

result = ktxTexture_CreateFromNamedFile("mytex3d.ktx",
                                        KTX_TEXTURE_CREATE_NO_FLAGS,
                                        &kTexture);

// Before the first call to  ktxTexture_GLUpload make libktx load its
// function pointers for the GL functions it uses. The parameter is a
// pointer to the GLGetProcAddress function provided by whatever OpenGL
// framework the application is using. The example shown is for SDL.
//
// Note 1: This is unrelated to any GL function pointers the app may be
//         using.
// Note 2: When this is not called, libktx has fallback mechanisms to
//         find the pointers which work on the vast majority of
//         platforms. The only known failures have occurred on Fedora.
result = ktxLoadOpenGL((PFNGLGETPROCADDRESS)SDL_GL_GetProcAddress);

glGenTextures(1, &texture); // Optional. GLUpload can generate a texture.
result = ktxTexture_GLUpload(kTexture, &texture, &target, &glerror);
ktxTexture_Destroy(kTexture);
// ...
// GL rendering using the texture
// ...
~~~~~~~~~~~~~~~~

## Creating a Vulkan image object from a KTX file.  {#createVulkan}

~~~~~~~~~~~~~~~~{.c}
#include <vulkan/vulkan.h>    // From your Vulkan SDK.
#include <ktxvulkan.h>

ktxTexture* kTexture = NULL;
KTX_error_code result;
ktx_size_t offset;
ktx_uint8_t* image;
ktx_uint32_t level, layer, faceSlice;
ktxVulkanDeviceInfo vdi;
ktxVulkanTexture texture;

// Set up Vulkan physical device (gpu), logical device (device), queue
// and command pool. Save the handles to these in a struct called vkctx.
// ktx VulkanDeviceInfo is used to pass these with the expectation that
// apps are likely to upload a large number of textures.
ktxVulkanDeviceInfo_Construct(&vdi, vkctx.gpu, vkctx.device,
                              vkctx.queue, vkctx.commandPool, nullptr);

result = ktxTexture_CreateFromNamedFile("mytex3d.ktx",
                                        KTX_TEXTURE_CREATE_NO_FLAGS,
                                        &kTexture);

result = ktxTexture_VkUploadEx(kTexture, &vdi, &texture,
                               VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_SAMPLED_BIT,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

ktxTexture_Destroy(kTexture);
ktxVulkanDeviceInfo_Destruct(&vdi);
// ...
// Vulkan rendering using the texture
// ...
// When done using the image in Vulkan...
ktxVulkanTexture_Destruct(&texture, vkctx.device, nullptr);
~~~~~~~~~~~~~~~~

## Extracting Metadata        {#subsection}

Once a ktxTexture object has been created, metadata can be easily found
and extracted. The following can be added to any of the above.

~~~~~~~~~~~~~~~~{.c}
char* pValue;
uint32_t valueLen;
if (KTX_SUCCESS == ktxHashList_FindValue(&kTexture->kvDataHead,
                                          KTX_ORIENTATION_KEY,
                                          &valueLen, (void**)&pValue))
 {
      char s, t;

      if (sscanf(pValue, KTX_ORIENTATION2_FMT, &s, &t) == 2) {
         ...
      }
 }
~~~~~~~~~~~~~~~~

## Writing a KTX or KTX2 file         {#writektx}

~~~~~~~~~~~~~~~~{.c}
#include <ktx.h>
#include <vulkan/vulkan.h>              // From your Vulkan SDK

ktxTexture2* texture = NULL;            // For KTX2
//ktxTexture1* texture = NULL;          // For KTX
ktxTextureCreateInfo createInfo;
KTX_error_code result;
ktx_uint32_t level, layer, faceSlice;
FILE* src;
ktx_size_t srcSize;

createInfo.glInternalformat = GL_RGB8;         // Ignored if creating a ktxTexture2.
createInfo.vkFormat = VK_FORMAT_R8G8B8_UNORM;  // Ignored if creating a ktxTexture1.
createInfo.baseWidth = 2048;
createInfo.baseHeight = 1024;
createInfo.baseDepth = 16;
createInfo.numDimensions = 3.
// Note: it is not necessary to provide a full mipmap pyramid.
createInfo.numLevels = log2(createInfo.baseWidth) + 1
createInfo.numLayers = 1;
createInfo.numFaces = 1;
createInfo.isArray = KTX_FALSE;
createInfo.generateMipmaps = KTX_FALSE;

// Call ktxTexture1_Create to create a KTX texture.
result = ktxTexture2_Create(&createInfo,
                            KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                            &texture);

src = // Open a stdio FILE* on the baseLevel image, slice 0.
srcSize = // Query size of the file.
level = 0;
layer = 0;
faceSlice = 0;                           
result = ktxTexture_SetImageFromMemory(ktxTexture(texture),
                                       level, layer, faceSlice,
                                       src, srcSize);
// Repeat for the other 15 slices of the base level and all other levels
// up to createInfo.numLevels.

result = ktxTexture_WriteToNamedFile(ktxTexture(texture), "mytex3d.ktx");
ktxTexture_Destroy(ktxTexture(texture));
~~~~~~~~~~~~~~~~

## Modifying a KTX file         {#modifyktx}

~~~~~~~~~~~~~~~~{.c}
#include <ktx.h>

ktxTexture* texture = NULL;
KTX_error_code result;

result = ktxTexture_CreateFromNamedFile("mytex3d.ktx",
                                        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                        &texture);
// The file is closed after all the data has been read.

// It is the responsibilty of the application to make sure its modifications
// are valid.
texture->generateMipmaps = KTX_TRUE;

result = ktxTexture_WriteToNamedFile(texture, "mytex3d.ktx");
ktxTexture_Destroy(texture);
~~~~~~~~~~~~~~~~

## Writing a Basis-compressed Universal Texture

Basis compression supports two universal texture formats: _BasisLZ/ETC1S_ and
_UASTC_. The latter gives higher quality at a larger file size. Textures can be
compressed to either format using `ktxTexture2_CompressBasisEx` as shown in this
example:

~~~~~~~~~~~~~~~~{.c}
#include <ktx.h>
#include <vulkan/vulkan.h>              // From your Vulkan SDK

ktxTexture2* texture = NULL;
ktxTextureCreateInfo createInfo;
KTX_error_code result;
ktx_uint32_t level, layer, faceSlice;
FILE* src;
ktx_size_t srcSize;
ktxBasisParams params = {0};
params.structSize = sizeof(params);

createInfo.glInternalformat = 0;  //Ignored as we'll create a KTX2 texture.
createInfo.vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
createInfo.baseWidth = 2048;
createInfo.baseHeight = 1024;
createInfo.baseDepth = 16;
createInfo.numDimensions = 3.
// Note: it is not necessary to provide a full mipmap pyramid.
createInfo.numLevels = log2(createInfo.baseWidth) + 1
createInfo.numLayers = 1;
createInfo.numFaces = 1;
createInfo.isArray = KTX_FALSE;
createInfo.generateMipmaps = KTX_FALSE;

result = ktxTexture2_Create(&createInfo,
                            KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                            &texture);

src = // Open the file for the baseLevel image, slice 0 and
      // read it into memory.
srcSize = // Query size of the file.
level = 0;
layer = 0;
faceSlice = 0;                           
result = ktxTexture_SetImageFromMemory(ktxTexture(texture),
                                       level, layer, faceSlice,
                                       src, srcSize);
// Repeat for the other 15 slices of the base level and all other levels
// up to createInfo.numLevels.

// For BasisLZ/ETC1S
params.compressionLevel = KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL;
// For UASTC
params.uastc = KTX_TRUE;
// Set other BasisLZ/ETC1S or UASTC params to change default quality settings.
result = ktxTexture2_CompressBasisEx(texture, &params);

result = ktxTexture_WriteToNamedFile(ktxTexture(texture), "mytex3d.ktx2");
ktxTexture_Destroy(ktxTexture(texture));
~~~~~~~~~~~~~~~~

There is a shortcut that can be used when compressing to BasisLZ/ETC1S. Remove
the declaration and initialization of `params` in the previous example and
replace @ref ktxTexture2::ktxTexture2_CompressBasisEx
with @ref ktxTexture2::ktxTexture2_CompressBasis as follows:

~~~~~~~~~~~~~~~~{.c}
// Quality range is 1 - 255. 0 gets the default quality, currently 128.
// The qualityLevel field in ktxBasisParams is set from this.
int quality = 0;
result = ktxTexture2_CompressBasis(texture, quality);
~~~~~~~~~~~~~~~~

## Transcoding a BasisLZ/ETC1S or UASTC-compressed Texture

~~~~~~~~~~~~~~~~{.c}
#include <ktx.h>

ktxTexture2* texture = NULL;
KTX_error_code result;

result = ktxTexture_CreateFromNamedFile("mytex3d_basis.ktx2",
                                        KTX_TEXTURE_CREATE_NO_FLAGS,
                                        (ktxTexture**)&kTexture);
// or
//result = ktxTexture2_CreateFromNamedFile("mytex3d_basis.ktx2",
//                                         KTX_TEXTURE_CREATE_NO_FLAGS,
//                                         &kTexture);

if (ktxTexture2_NeedsTranscoding(texture)) {
    ktx_texture_transcode_fmt_e tf;

    // Using VkGetPhysicalDeviceFeatures or GL_COMPRESSED_TEXTURE_FORMATS or
    // extension queries, determine what compressed texture formats are
    // supported and pick a format. For example
    vk::PhysicalDeviceFeatures deviceFeatures;
    vkctx.gpu.getFeatures(&deviceFeatures);
    khr_df_model_e colorModel = ktxTexture2_GetColorModel_e(texture);
    if (colorModel == KHR_DF_MODEL_UASTC
        && deviceFeatures.textureCompressionASTC_LDR) {
        tf = KTX_TTF_ASTC_4x4_RGBA;
    } else if (colorModel == KHR_DF_MODEL_ETC1S
               && deviceFeatures.textureCompressionETC2) {
        tf = KTX_TTF_ETC;
    } else if (deviceFeatures.textureCompressionASTC_LDR) {
        tf = KTX_TTF_ASTC_4x4_RGBA;
    } else if (deviceFeatures.textureCompressionETC2)
        tf = KTX_TTF_ETC2_RGBA;
    else if (deviceFeatures.textureCompressionBC)
        tf = KTX_TTF_BC3_RGBA;
    else {
        printf("Vulkan implementation does not support any available transcode target.");
        // return error or throw exception (if C++) ...
    }

    result = ktxTexture2_TranscodeBasis(texture, tf, 0);

    // Then use VkUpload or GLUpload to create a texture object on the GPU.
}
~~~~~~~~~~~~~~~~

## Writing an ASTC-Compressed Texture

The following example showcases how to use libktx to generate an ASTC-compressed
texture using @ref ktxTexture2::ktxTexture2_CompressAstcEx.

You can also use Basis Universal's @ref ktxTexture2::ktxTexture2\_TranscodeBasis
to transcode UASTC, BasisLZ/ETC1S, or other supported codecs to ASTC but the
difference here is that this directly encodes to a target ASTC format without
any intermediate steps (i.e., transcoding) that are likely to introduce more
artifacts.

~~~~~~~~~~~~~~~~{.c}
#include <ktx.h>
#include <vulkan/vulkan.h>              // From your Vulkan SDK

ktxTexture2* texture = NULL;
ktxTextureCreateInfo createInfo;
KTX_error_code result;                  // Stores libktx success/error results
ktxAstcParams params = {0};             // Make sure to 0-initialize this struct
params.structSize = sizeof(params);

createInfo.glInternalformat = 0;  // Ignored as we'll create a KTX2 texture.
createInfo.vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
createInfo.baseWidth = 2048;
createInfo.baseHeight = 1024;
createInfo.baseDepth = 16;
createInfo.numDimensions = 3.
// Note: it is not necessary to provide a full mipmap pyramid.
createInfo.numLevels = log2(createInfo.baseWidth) + 1
createInfo.numLayers = 1;
createInfo.numFaces = 1;
createInfo.isArray = KTX_FALSE;
createInfo.generateMipmaps = KTX_FALSE;

result = ktxTexture2_Create(&createInfo,
                            KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                            &texture);
if (result != KTX_SUCCESS) {
    // error handling ...
}

// Set uncompressed image data for each mip level, each array layer, and each
// face/depth slice
for (uint32_t level = 0; level < texture->numLevels; ++level) {
    // Compute the width/height/depth for this MIP level
    const uint32_t width = MAX(texture->baseWidth >> level, 1u);
    const uint32_t height = MAX(texture->baseHeight >> level, 1u);
    const uint32_t depth = MAX(texture->baseDepth >> level, 1u);
    // Query/Compute the size of this level image
    const ktx_size_t srcSize = 0;
    for (uint32_t layer = 0; layer < texture->numLayers; ++layer) {
        for (uint32_t face = 0; face < texture->numFaces; ++face) {
            for (uint32_t depthSlice = 0; depthSlice < depth; ++depthSlice) {
                // Read file for this layer and face/slice (or generate from
                // base level image)
                ktx_uint8_t* src = NULL;
                // Set uncompressed image data. Adding face and depth slice
                // indices is fine because they are mutually exclusive (if one
                // is > 0 the other is guaranteed to be 0)
                result = ktxTexture_SetImageFromMemory(ktxTexture(texture),
                                                       level, layer,
                                                       face + depthSlice,
                                                       src, srcSize);
                if (result != KTX_SUCCESS) {
                    // error handling and ktx texture cleanup ...
                }
            }
        }
    }
}

// Compress the whole previously set uncompressed data into ASTC
params.threadCount = 1;
params.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_6x6;
params.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
params.qualityLevel = KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM;
result = ktxTexture2_CompressAstcEx(texture, &params);
if (result != KTX_SUCCESS) {
    // error handling and ktx texture cleanup ...
}

result = ktxTexture_WriteToNamedFile(ktxTexture(texture), "mytex3d.ktx2");
if (result != KTX_SUCCESS) {
    // error handling and ktx texture cleanup ...
}

ktxTexture_Destroy(ktxTexture(texture));
~~~~~~~~~~~~~~~~

There is a shortcut that can be used when the only `params` field you want to
modify is the `qualityLevel`. Remove the declaration and initialization of
`params` in the previous example and replace
@ref ktxTexture2::ktxTexture2_CompressAstcEx with
@ref ktxTexture2::ktxTexture2_CompressAstc as follows:

~~~~~~~~~~~~~~~~{.c}
// Quality range is 0 - 100. 0 is fastest/lowest. 100 is slowest/highest.
int quality = KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM;
result = ktxTexture2_CompressAstc(texture, quality);
~~~~~~~~~~~~~~~~

## Writing a BCn-Compressed Texture

The following example showcases how to use libktx to generate a BCn-compressed
(i.e., BC1, BC3, BC4, BC5, BC6HU, BC6HS, or BC7) texture using @ref
ktxTexture2::ktxTexture2\_CompressBCnEx.

You can also use Basis Universal's @ref ktxTexture2::ktxTexture2\_TranscodeBasis
to transcode UASTC (or other supported codecs) to BCn but the difference here is
that this directly encodes to a target BCn format without any intermediate steps
(i.e., transcoding) that are very likely to introduce more artifacts.

This example is kept as simple as possible. There are a lot of other parameters
that are only activated depending on the target BCn scheme and whether rate
distortion optimization (RDO) is enabled. The discussion about RDO parameter
details is too involved for this example (see member parameters descriptions
in @ref ktxBCnParams struct).

~~~~~~~~~~~~~~~~{.c}
#include <ktx.h>
#include <vulkan/vulkan.h>              // From your Vulkan SDK

ktxTexture2* texture = NULL;            // KTX2 texture handle
ktxTextureCreateInfo createInfo;
KTX_error_code result;                  // Stores libktx success/error results
ktxBCnParams params = {0};              // Make sure to 0-initialize this struct

// Fill up texture creation struct. Notice that the supplied vkFormat must be
// an uncompressed format that matches the target BCn that is set later. In this
// example, BC7 expects RGBA input.
createInfo.glInternalformat = 0;  // Ignored as we'll create a KTX2 texture.
createInfo.vkFormat = VK_FORMAT_R8G8B8A8_SRGB;
createInfo.baseWidth = 1024;
createInfo.baseHeight = 1024;
createInfo.baseDepth = 1;
createInfo.numDimensions = 1;
// Note: it is not necessary to provide a full mipmap pyramid.
createInfo.numLevels = log2(createInfo.baseWidth) + 1;
createInfo.numLayers = 1;
createInfo.numFaces = 1;
createInfo.isArray = KTX_FALSE;
createInfo.generateMipmaps = KTX_FALSE;

result = ktxTexture2_Create(&createInfo,
                            KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                            &texture);
if (result != KTX_SUCCESS) {
    // error handling ...
}

// Set uncompressed image data for each mip level, each array layer, and each
// face/depth slice
for (uint32_t level = 0; level < texture->numLevels; ++level) {
    // Compute the width/height/depth for this MIP level
    const uint32_t width = MAX(texture->baseWidth >> level, 1u);
    const uint32_t height = MAX(texture->baseHeight >> level, 1u);
    const uint32_t depth = MAX(texture->baseDepth >> level, 1u);
    // Query/Compute the size of this level image
    const ktx_size_t srcSize = 0;
    for (uint32_t layer = 0; layer < texture->numLayers; ++layer) {
        for (uint32_t face = 0; face < texture->numFaces; ++face) {
            for (uint32_t depthSlice = 0; depthSlice < depth; ++depthSlice) {
                // Read file for this layer and face/slice (or generate from
                // base level image)
                ktx_uint8_t* src = NULL;
                // Set uncompressed image data. Adding face and depth slice
                // indices is fine because they are mutually exclusive (if one
                // is > 0 the other is guaranteed to be 0)
                result = ktxTexture_SetImageFromMemory(ktxTexture(texture),
                                                       level, layer,
                                                       face + depthSlice,
                                                       src, srcSize);
                if (result != KTX_SUCCESS) {
                    // error handling and ktx texture cleanup ...
                }
            }
        }
    }
}

// Now that uncompressed image data is set, compress to BCn:
params.structSize = sizeof(params);
params.bcn = KTX_BCN_COMPRESSION_BC7;
params.bcnCompressionQuality = KTX_PACK_BCN_QUALITY_LEVEL_MEDIUM;

// See ktxTexture2::ktxTexture2_CompressBCnEx
result = ktxTexture2_CompressBCnEx(texture, &params);
if (result != KTX_SUCCESS) {
    // error handling and ktx texture cleanup ...
}

// You may also apply another compression layer (supercompression) to further
// reduce filesize (up-to ~50% if RDO is set) at the expense of little CPU
// overhead when loading the texture:
result = ktxTexture2_DeflateZstd(texture, 22 /* ZSTD level [0, 22] */);
if (result != KTX_SUCCESS) {
    // error handling and ktx texture cleanup ...
}

result = ktxTexture_WriteToNamedFile(ktxTexture(texture), "bc7_texture.ktx2");
if (result != KTX_SUCCESS) {
    // error handling and ktx texture cleanup ...
}

// Don't forget to destroy the texture (this will clean-up all allocated
// resources)
ktxTexture_Destroy(ktxTexture(texture));
~~~~~~~~~~~~~~~~

You may notice that there are `ktxTexture2` alternatives for the above
textures that take `ktxTexture2*`. These can also be used instead of the
last ones. Both fall down to calling the same approriate function on the
supplied KTX texture (either ktxTexture1 or ktxTexture2). This is achieved
via virtual table pointer in ktxTexture C struct.
