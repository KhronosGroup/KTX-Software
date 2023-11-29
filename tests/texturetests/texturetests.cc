/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/**
 * @internal
 * @file texturetests.cc
 * @~English
 *
 * @brief Test ktxTexture API functions.
 *
 * @author Mark Callow, Edgewise Consulting
 */

/*
 * Copyright 2010-2020 Mark Callow, <khronos at callow dot im>.
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(_WIN32)
  #define _CRT_SECURE_NO_WARNINGS
  #if _MSC_VER < 1900
    #define snprintf _snprintf
  #endif
#endif

#include <string>
//#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include "GL/glcorearb.h"
#include "ktx.h"
#include "ktxint.h"
#include "texture.h"
#include "texture1.h"
#include "texture2.h"
#include "gtest/gtest.h"
#include "wthelper.h"
#include "vk_format.h"

#define ROUNDING(x) \
        (3 - ((x + KTX_GL_UNPACK_ALIGNMENT-1) % KTX_GL_UNPACK_ALIGNMENT));

#if defined(TestNoMetadata)
extern ktx_bool_t __disableWriterMetadata__;
#endif

namespace {

// Recursive function to return the greatest common divisor of a and b.
static uint32_t
gcd(uint32_t a, uint32_t b) {
    if (a == 0)
        return b;
    return gcd(b % a, a);
}

// Function to return the least common multiple of a & 4.
uint32_t
lcm4(uint32_t a)
{
    if (!(a & 0x03))
        return a;  // a is a multiple of 4.
    return (a*4) / gcd(a, 4);
}

//-------------------------------------------------------
// Helper for base fixture & ktxTexture_WriterTest cases.
//-------------------------------------------------------

template<typename component_type, ktx_uint32_t numComponents,
         GLenum internalformat>
class TextureWriterTestHelper
    : public WriterTestHelper<component_type, numComponents, internalformat> {
  public:
    using typename WriterTestHelper<component_type, numComponents, internalformat>::createFlags;
    using typename WriterTestHelper<component_type, numComponents, internalformat>::createFlagBits;
    using WriterTestHelper<component_type, numComponents, internalformat>::images;
    using WriterTestHelper<component_type, numComponents, internalformat>::imageList;
    using WriterTestHelper<component_type, numComponents, internalformat>::width;
    using WriterTestHelper<component_type, numComponents, internalformat>::height;
    using WriterTestHelper<component_type, numComponents, internalformat>::levelsFromSize;

    TextureWriterTestHelper() {}

    void
    resize(createFlags flags, ktx_uint32_t layers, ktx_uint32_t faces,
           ktx_uint32_t dimensions,
           ktx_uint32_t w, ktx_uint32_t h, ktx_uint32_t d)
    {
        WriterTestHelper<component_type, numComponents, internalformat>::resize(
                                               flags, layers, faces,
                                               dimensions,
                                               w, h, d);
        createInfo.resize(flags, layers, faces,
                          dimensions, w, h, d);
    }

    // Compare images as loaded into a ktxTexture1 object with our image.
    bool
    compareTexture1Images(ktx_uint8_t* pData)
    {
        for (ktx_uint32_t level = 0; level < images.size(); level++) {
            ktx_uint32_t levelWidth = MAX(1, width >> level);
            ktx_uint32_t levelHeight = MAX(1, height >> level);
            ktx_size_t rowBytes = levelWidth * sizeof(component_type) * numComponents;
            ktx_uint32_t rowPadding = ROUNDING(rowBytes);
            ktx_size_t paddedImageBytes = (rowBytes + rowPadding) * levelHeight;
            for (ktx_uint32_t layer = 0; layer < images[0].size(); layer++) {
                for (ktx_uint32_t faceSlice = 0; faceSlice < images[level][layer].size(); faceSlice++) {
                    if (rowPadding == 0) {
                        if (memcmp(images[level][layer][faceSlice].data(), pData,
                                   images[level][layer][faceSlice].size() * sizeof(component_type)))
                            return false;
                        pData += paddedImageBytes;
                    } else {
                        ktx_uint8_t* pImage = (ktx_uint8_t*)images[level][layer][faceSlice].data();
                        for (ktx_uint32_t row = 0; row < levelHeight; row++) {
                            if (memcmp(pImage, pData, rowBytes))
                                return false;
                            pImage += rowBytes;
                            pData += rowBytes + rowPadding;
                        }
                    }
                }
            }
        }
        return true;
    }

    // Compare images as loaded into a ktxTexture2 object with our image.
    bool
    compareTexture2Images(ktx_uint8_t* pData)
    {
        for (ktx_int32_t level = (ktx_int32_t)images.size() - 1; level >= 0; level--) {
            ktx_uint32_t levelWidth = MAX(1, width >> level);
            ktx_uint32_t levelHeight = MAX(1, height >> level);
            ktx_uint32_t texelBlockSize = sizeof(component_type) * numComponents;
            ktx_uint32_t requiredLevelAlignment = lcm4(texelBlockSize);
            ktx_size_t rowBytes = levelWidth * texelBlockSize;
            ktx_size_t imageBytes = rowBytes * levelHeight;
            for (ktx_uint32_t layer = 0; layer < images[0].size(); layer++) {
                for (ktx_uint32_t faceSlice = 0; faceSlice < images[level][layer].size(); faceSlice++) {
                    if (memcmp(images[level][layer][faceSlice].data(), pData,
                               images[level][layer][faceSlice].size() * sizeof(component_type)))
                        return false;
                    pData += imageBytes;
                }
            }
            pData += _KTX_PADN_LEN(requiredLevelAlignment, imageBytes);
        }
        return true;
    }

    KTX_error_code
    copyImagesToTexture(ktxTexture1* texture) {
        KTX_error_code result = KTX_SUCCESS;

        for (ktx_uint32_t level = 0; level < images.size(); level++) {
            for (ktx_uint32_t layer = 0; layer < images[level].size(); layer++) {
                for (ktx_uint32_t faceSlice = 0; faceSlice < images[level][layer].size(); faceSlice++) {
                    ktx_size_t imageBytes = images[level][layer][faceSlice].size() * sizeof(component_type);
                    ktx_uint8_t* imageDataPtr = (ktx_uint8_t*)(images[level][layer][faceSlice].data());
                    result = ktxTexture1_SetImageFromMemory(texture,
                                                            level, layer,
                                                            faceSlice,
                                                            imageDataPtr,
                                                            imageBytes);
                   if (result != KTX_SUCCESS)
                       break;
                }
            }
        }
        return result;
    }

    class createInfo : public ktxTextureCreateInfo {
      public:
        createInfo() {
            glInternalformat = internalformat;
        }

        void resize(createFlags flags,
                    ktx_uint32_t layers, ktx_uint32_t faces,
                    ktx_uint32_t dimensions, ktx_uint32_t w,
                    ktx_uint32_t h, ktx_uint32_t d)
        {
            baseWidth = w;
            baseHeight = h;
            baseDepth = d;
            this->numDimensions = dimensions;
            generateMipmaps = flags & createFlagBits::eGenerateMipmaps
                              ? KTX_TRUE : KTX_FALSE;
            isArray = flags & createFlagBits::eArray ? KTX_TRUE : KTX_FALSE;
            this->numFaces = faces;
            this->numLayers = layers;
            numLevels = flags & createFlagBits::eMipmapped
                        ? levelsFromSize(w, h, d) : 1;
        };
    } createInfo;

};

const ktx_uint8_t ktxId[12] = KTX_IDENTIFIER_REF;
const ktx_uint8_t ktxId2[12] = KTX2_IDENTIFIER_REF;

///////////////////////////////////////////////////////////
// Test fixtures
///////////////////////////////////////////////////////////

//----------------------------------------------------
// Base fixture for ktxTexture and related test cases.
//----------------------------------------------------

typedef TextureWriterTestHelper<GLubyte, 4, GL_RGBA8>::createFlagBits createFlagBits;

template<typename component_type, ktx_uint32_t numComponents,
         GLenum internalformat>
class ktxTextureTestBase : public ::testing::Test {
  protected:
    ktxTextureTestBase(ktxFormatVersionEnum fv) : pixelSize(16)
    {
        helper.resize(createFlagBits::eMipmapped, 1, 1, 2, 16, 16, 1);
        // Create a KTX file in memory for testing.

        KTX_error_code errorCode;

        ktxMemFile = 0;
        iterCbCalls = 0;

        mipLevels = helper.numLevels;

        // Create the in-memory KTX file

        ktxTexture* texture = 0;
        if (fv == KTX_FORMAT_VERSION_ONE) {
           kvDataLen = helper.kvDataLen;
           kvData = helper.kvData;
           errorCode = ktxTexture1_Create(&texinfo,
                                           KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                           (ktxTexture1**)&texture);
        } else {
            kvDataLen = helper.kvDataLenWriter_ktx2;
            kvData = helper.kvDataWriter_ktx2;
            texinfo.vkFormat
                = vkGetFormatFromOpenGLInternalFormat(texinfo.glInternalformat);
            errorCode = ktxTexture2_Create(&texinfo,
                                           KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                           (ktxTexture2**)&texture);
            texture->kvDataHead = helper.kvHash_ktx2;
        }
        if (KTX_SUCCESS != errorCode) {
            ADD_FAILURE() << "ktxTexture"
                          << (fv == KTX_FORMAT_VERSION_ONE ? "1" : "2")
                          << "_Create failed: "
                          << ktxErrorString(errorCode);
            return;
        }

        // Don't use the above helper.copyImagesToTexture because that is used
        // by various test cases which will compare their results against this.
        // A different code path here provides a small extra correctness check.
        std::vector<wthImageInfo>::const_iterator it = images.begin();
        for (ktx_uint32_t level = 0; level < texinfo.numLevels; level++) {
            ktx_uint32_t levelDepth = MAX(1, texinfo.baseDepth >> level);
            for (ktx_uint32_t layer = 0; layer < texinfo.numLayers; layer++) {
                ktx_uint32_t numImages = texinfo.numFaces == 6
                                       ? texinfo.numFaces : levelDepth;
                for (ktx_uint32_t faceSlice = 0; faceSlice < numImages; faceSlice++) {
                    ktxTexture_SetImageFromMemory(texture,
                                                  level, layer, faceSlice,
                                                  it->data, it->size);
                }
            }
            it++;
        }

        paddedImageDataSize = texture->dataSize;
        texture->kvData = kvData;
        texture->kvDataLen = kvDataLen;
        errorCode = ktxTexture_WriteToMemory(texture, &ktxMemFile,
                                             &ktxMemFileLen);
        if (KTX_SUCCESS != errorCode) {
            ADD_FAILURE() << "ktxTexture_WriteToMemory failed: "
                          << ktxErrorString(errorCode);
        }
    }

    ~ktxTextureTestBase() {
        delete ktxMemFile;
    }

    KTX_error_code
    iterCallback(int miplevel, int /*face*/,
                 int width, int /*height*/, int /*depth*/,
                 ktx_uint64_t faceLodSize,
                 void* pixels)
    {
        int expectedWidth = pixelSize >> miplevel;
        EXPECT_EQ(width, expectedWidth);
        EXPECT_EQ(faceLodSize, (uint64_t)(expectedWidth * expectedWidth * 4));
        EXPECT_EQ(memcmp(pixels, images[miplevel].data, images[miplevel].size),
                  0);
        iterCbCalls++;
        return KTX_SUCCESS;
    }

    static KTX_error_code
    iterCallback(int miplevel, int face,
                  int width, int height, int depth,
                  ktx_uint64_t faceLodSize,
                  void* pixels, void* userdata)
    {
        ktxTextureTestBase* fixture = (ktxTextureTestBase*)userdata;
        return fixture->iterCallback(miplevel, face, width, height, depth,
                                     faceLodSize, pixels);
    }

    TextureWriterTestHelper<component_type, numComponents, internalformat> helper;
    wthTexInfo& texinfo = helper.texinfo;
    ktxTextureCreateInfo& createInfo = helper.createInfo;
    unsigned char* kvData;
    unsigned int kvDataLen;

    ktx_uint8_t* ktxMemFile;
    ktx_size_t ktxMemFileLen;
    const int pixelSize;
    unsigned int mipLevels;
    unsigned int iterCbCalls;

    ktx_size_t paddedImageDataSize;
    ktx_size_t& imageDataSize = helper.imageDataSize;
    std::vector< std::vector < std::vector < std::vector<GLubyte>  > > >& imageData = helper.images;

    std::vector<wthImageInfo>& images = helper.imageList;
};

class ktxTexture1TestBase : public ktxTextureTestBase<GLubyte, 4, GL_RGBA8> {
  protected:
    ktxTexture1TestBase() : ktxTextureTestBase(KTX_FORMAT_VERSION_ONE) { }

    bool
    compareTexture(ktxTexture1* texture)
    {
        if (texture->glInternalformat != texinfo.glInternalformat)
            return false;
        if (texture->glBaseInternalformat != texinfo.glBaseInternalformat)
            return false;
        if (texture->glFormat != texinfo.glFormat)
            return false;
        if (texture->glType != texinfo.glType)
            return false;
        if (ktxTexture1_glTypeSize(texture) != texinfo.glTypeSize)
            return false;
        if (texture->baseWidth != texinfo.baseWidth)
            return false;
        if (texinfo.baseHeight == 0) {
            if (texture->baseHeight != 1)
                return false;
        } else if (texture->baseHeight != texinfo.baseHeight)
            return false;
        if (texinfo.baseDepth == 0) {
            if (texture->baseDepth != 1)
                return false;
        } else if (texture->baseDepth != texinfo.baseDepth)
            return false;
        if (texture->numFaces != texinfo.numFaces)
            return false;
        if (texture->numLevels != texinfo.numLevels)
            return false;
        return true;
    }
};


template<typename component_type,
         ktx_uint32_t numComponents,
         GLenum internalformat>
class ktxTexture2TestBase : public ktxTextureTestBase<component_type,
                                                      numComponents,
                                                      internalformat>
{
  protected:
    using ktxTextureTestBase<component_type, numComponents, internalformat>::helper;
    using ktxTextureTestBase<component_type, numComponents, internalformat>::texinfo;

    ktxTexture2TestBase() : ktxTextureTestBase<component_type, numComponents, internalformat>(KTX_FORMAT_VERSION_TWO) { }

    bool
    compareTexture(ktxTexture2* texture)
    {
        if (texture->vkFormat != (uint32_t)vkGetFormatFromOpenGLInternalFormat(helper.texinfo.glInternalformat))
            return false;
        if (texture->baseWidth != texinfo.baseWidth)
            return false;
        if (texinfo.baseHeight == 0) {
            if (texture->baseHeight != 1)
                return false;
        } else if (texture->baseHeight != texinfo.baseHeight)
            return false;
        if (texinfo.baseDepth == 0) {
            if (texture->baseDepth != 1)
                return false;
        } else if (texture->baseDepth != texinfo.baseDepth)
            return false;
        if (texture->numFaces != texinfo.numFaces)
            return false;
        if (texture->numLevels != texinfo.numLevels)
            return false;
        return true;
    }
};

class ktxTexture2_CreateTest : public ::testing::Test {
  protected:
    ktx_error_code_e create(VkFormat format,
                          ktx_uint32_t width = 16u,
                          ktx_uint32_t height = 16u,
                          ktx_uint32_t depth = 1u,
                          ktx_uint32_t dimensions = 2u,
                          ktx_uint32_t levels = 1u,
                          ktx_uint32_t layers = 1u,
                          ktx_uint32_t faces = 1u,
                          bool isArray = false,
                          bool generateMipmaps = false)
    {
        ktxTextureCreateInfo createInfo;
            createInfo.vkFormat = format;
            createInfo.baseWidth = width;
            createInfo.baseHeight = height;
            createInfo.baseDepth = depth;
            createInfo.numDimensions = dimensions;
            createInfo.numLevels = levels;
            createInfo.numLayers = layers;
            createInfo.numFaces = faces;
            createInfo.isArray = isArray;
            createInfo.generateMipmaps = generateMipmaps;

            return ktxTexture2_Create(&createInfo,
                                      KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                      &texture);
    }

    ~ktxTexture2_CreateTest() {
        ktxTexture_Destroy(ktxTexture(texture));
    }

    ktxTexture2* texture;
};

//----------------------------------------------------
// Template for base fixture for ktxTextureWrite tests.
//----------------------------------------------------

template<typename component_type, ktx_uint32_t numComponents,
         GLenum internalformat>
class ktxTexture1WriteTestBase : public ::testing::Test {
  public:
    using createFlags = typename WriterTestHelper<component_type, numComponents, internalformat>::createFlags;
    ktxTexture1WriteTestBase() { }

    void runTest(bool writeMetadata) {
        ktxTexture1* texture = 0;
        KTX_error_code result;
        ktx_uint8_t* ktxMemFile;
        ktx_size_t ktxMemFileLen;
        ktx_uint8_t* filePtr;

        result = ktxTexture1_Create(&helper.createInfo,
                                   KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                   &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture1_Create failed: "
                                     << ktxErrorString(result);

        if (writeMetadata)
            ktxHashList_AddKVPair(&texture->kvDataHead, KTX_ORIENTATION_KEY,
                                  (unsigned int)strlen(helper.orientation) + 1,
                                  helper.orientation);

        result = helper.copyImagesToTexture(texture);
        ASSERT_TRUE(result == KTX_SUCCESS);

        EXPECT_EQ(helper.compareTexture1Images(texture->pData), true);
        result = ktxTexture1_WriteToMemory(texture, &ktxMemFile, &ktxMemFileLen);

        ASSERT_TRUE(result == KTX_SUCCESS) << "ktxTexture_WriteToMemory failed: "
                                           << ktxErrorString(result);
        EXPECT_EQ(memcmp(ktxMemFile, ktxId, sizeof(ktxId)), 0);
        EXPECT_EQ(helper.texinfo.compare((KTX_header*)ktxMemFile), true);

        // Check the metadata.
        filePtr = ktxMemFile + sizeof(KTX_header);
        if (writeMetadata) {
            EXPECT_EQ(memcmp(filePtr, helper.kvData, helper.kvDataLen), 0);
            filePtr += helper.kvDataLen;
        }
        // Check data pointer is properly aligned.
        EXPECT_EQ((intptr_t)filePtr & 0x3, 0);

        EXPECT_EQ(helper.compareRawImages(filePtr), true);

        delete ktxMemFile;
        ktxTexture1_Destroy(texture);
    }

    TextureWriterTestHelper<component_type, numComponents, internalformat> helper;
};

//---------------------------
// Actual test fixtures
//---------------------------

class ktxTexture_CreateTest : public ktxTexture1TestBase { };
class ktxTexture1_CreateTest : public ktxTexture1TestBase { };
class ktxTexture_KVDataTest : public ktxTexture1TestBase { };
class ktxTexture1_IterateLoadLevelFacesTest : public ktxTexture1TestBase { };
class ktxTexture1_IterateLevelFacesTest : public ktxTexture1TestBase { };
class ktxTexture1_LoadImageDataTest : public ktxTexture1TestBase { };

class ktxTexture1WriteTestRGBA8 : public ktxTexture1WriteTestBase<GLubyte, 4, GL_RGBA8> { };
class ktxTexture1WriteTestRGB8 : public ktxTexture1WriteTestBase<GLubyte, 3, GL_RGB8> { };
class ktxTexture1WriteTestRG16 : public ktxTexture1WriteTestBase<GLushort, 2, GL_RG16> { };

class ktxTexture2_IterateLoadLevelFacesTest : public ktxTexture2TestBase<GLubyte, 4, GL_RGBA8>  { };
class ktxTexture2_IterateLevelFacesTest : public ktxTexture2TestBase<GLubyte, 4, GL_RGBA8>  { };
class ktxTexture2_IterateLevelsTest : public ktxTexture2TestBase<GLubyte, 4, GL_RGBA8> { };
class ktxTexture2_LoadImageDataTest : public ktxTexture2TestBase<GLubyte, 4, GL_RGBA8> { };
class ktxTexture2_CreateCopyTest: public ktxTexture2TestBase<GLubyte, 4, GL_RGBA8> { };

/////////////////////////////////////////
// ktxTexture_Create tests
////////////////////////////////////////

TEST_F(ktxTexture1_CreateTest, InvalidValueOnNullParams) {
    ktxTexture* texture = 0;

    EXPECT_EQ(ktxTexture_CreateFromStdioStream(0, 0, &texture),
              KTX_INVALID_VALUE);
    EXPECT_EQ(ktxTexture_CreateFromNamedFile(0, 0, &texture),
              KTX_INVALID_VALUE);
    EXPECT_EQ(ktxTexture_CreateFromMemory(0, 0, 0, &texture),
              KTX_INVALID_VALUE);
    //EXPECT_EQ(ktxTexture_CreateFromStdioStream(0, 0, 0),
    //          KTX_INVALID_VALUE);
    EXPECT_EQ(ktxTexture_CreateFromNamedFile("foo", 0, 0),
              KTX_INVALID_VALUE);
    EXPECT_EQ(ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen, 0, 0),
              KTX_INVALID_VALUE);
}

TEST_F(ktxTexture_CreateTest, ConstructFromMemory) {
    ktxTexture* texture = 0;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             0, &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        EXPECT_EQ(texture->classId, ktxTexture1_c);
        EXPECT_EQ(compareTexture((ktxTexture1*)texture), true);
        EXPECT_EQ(texture->isCompressed, KTX_FALSE);
        EXPECT_EQ(texture->generateMipmaps, KTX_FALSE);
        EXPECT_EQ(texture->numDimensions, 2U);
        EXPECT_EQ(texture->numLayers, 1U);
        EXPECT_EQ(texture->isArray, KTX_FALSE);
        if (texture)
            ktxTexture_Destroy(texture);
    }
}

TEST_F(ktxTexture1_CreateTest, ConstructFromMemory) {
    ktxTexture1* texture = 0;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture1_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             0, &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        EXPECT_EQ(compareTexture(texture), true);
        EXPECT_EQ(texture->isCompressed, KTX_FALSE);
        EXPECT_EQ(texture->generateMipmaps, KTX_FALSE);
        EXPECT_EQ(texture->numDimensions, 2U);
        EXPECT_EQ(texture->numLayers, 1U);
        EXPECT_EQ(texture->isArray, KTX_FALSE);
        if (texture)
            ktxTexture1_Destroy(texture);
    }
}

TEST_F(ktxTexture1_CreateTest, CreateEmpty) {
    ktxTexture1* texture = 0;
    KTX_error_code result;

    result = ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                                &texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture1_Create failed: "
                                 << ktxErrorString(result);
    if (texture)
        ktxTexture1_Destroy(texture);
}

TEST_F(ktxTexture1_CreateTest, InvalidValueTooManyMipLevels) {
    ktxTexture1* texture = 0;

    createInfo.numLevels += 1;

    EXPECT_EQ(ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE, &texture),
              KTX_INVALID_OPERATION);
}

TEST_F(ktxTexture1_CreateTest, InvalidOpOnSetImagesNoStorage) {
    ktxTexture1* texture = 0;
    KTX_error_code result;

    result = ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                               &texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture1_Create failed: "
                                 << ktxErrorString(result);

    // Type RGBA UNSIGNED_BYTE -> *4
    ktx_size_t imageBytes = imageData[0].size() * 4;
    ktx_uint8_t* imageDataPtr = (ktx_uint8_t*)(&imageData[0].front());
    // Allocate the image data and initialize it to a color.
    EXPECT_EQ(ktxTexture1_SetImageFromMemory(texture, 0, 0, 0,
                                             imageDataPtr,
                                             imageBytes),
              KTX_INVALID_OPERATION);
    ASSERT_TRUE(result == KTX_SUCCESS);

    if (texture)
        ktxTexture1_Destroy(texture);
}

TEST_F(ktxTexture1_CreateTest, CreateEmptyAndSetImages) {
    ktxTexture1* texture = 0;
    KTX_error_code result;

    result = ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                &texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture1_Create failed: "
                                 << ktxErrorString(result);

    result = helper.copyImagesToTexture(texture);
    ASSERT_TRUE(result == KTX_SUCCESS);
    // imageData is an RGBA texture so no rounding is necessary and we can
    // use this simple comparison.
    EXPECT_EQ(helper.compareTexture1Images(texture->pData), true);

    if (texture)
        ktxTexture1_Destroy(texture);
}

TEST_F(ktxTexture1_CreateTest, CreateEmptySetImagesWriteToMemory) {
    ktxTexture1* texture = 0;
    KTX_error_code result;
    ktx_uint8_t* testMemFile;
    ktx_size_t testMemFileLen;
    char orientation[10];

    result = ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                &texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture1_Create failed: "
                                 << ktxErrorString(result);

    snprintf(orientation, sizeof(orientation), KTX_ORIENTATION2_FMT,
             'r', 'd');
    ktxHashList_AddKVPair(&texture->kvDataHead, KTX_ORIENTATION_KEY,
                          (unsigned int)strlen(orientation) + 1,
                          orientation);
    result = helper.copyImagesToTexture(texture);
    ASSERT_TRUE(result == KTX_SUCCESS);
    EXPECT_EQ(helper.compareTexture1Images(texture->pData), true);
    EXPECT_EQ(ktxTexture1_WriteToMemory(texture, &testMemFile, &testMemFileLen),
              KTX_SUCCESS);
    EXPECT_EQ(testMemFileLen, ktxMemFileLen);
    EXPECT_EQ(memcmp(testMemFile, ktxMemFile, ktxMemFileLen), 0);

    if (texture)
        ktxTexture1_Destroy(texture);
}

/////////////////////////////////////////
// ktxTexture2_Create tests
////////////////////////////////////////

TEST_F(ktxTexture2_CreateTest, E5B9G9R9) {
    ktx_error_code_e result = create(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32);
    EXPECT_EQ(result, KTX_SUCCESS);
}

/////////////////////////////////////////
// ktxTexture_KVData tests
////////////////////////////////////////

TEST_F(ktxTexture_KVDataTest, KVDataDeserialized) {
    ktxTexture* texture = 0;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             0,
                                             &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->kvData == NULL) << "Raw KVData should not be loaded";
        ASSERT_TRUE(texture->kvDataHead != NULL) << "KVData not deserialized";

        char* pValue;
        ktx_uint32_t valueLen;
        char s, t;
        result = ktxHashList_FindValue(&texture->kvDataHead,
                                       KTX_ORIENTATION_KEY,
                                       &valueLen, (void**)&pValue);
        EXPECT_EQ(result, KTX_SUCCESS);
        EXPECT_EQ(sscanf(pValue, /*valueLen,*/ KTX_ORIENTATION2_FMT, &s, &t), 2);
        EXPECT_EQ(s,'r');
        EXPECT_EQ(t, 'd');
        if (texture)
            ktxTexture_Destroy(texture);
    }
}

TEST_F(ktxTexture_KVDataTest, LoadRawKVData) {
    ktxTexture* texture = 0;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             KTX_TEXTURE_CREATE_RAW_KVDATA_BIT,
                                             &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
        << ktxErrorString(result);
        ASSERT_TRUE(texture->kvData != NULL) << "Raw KVData not loaded";
        ASSERT_TRUE(texture->kvDataHead == NULL) << "KVData should not be deserialized";
        EXPECT_EQ(texture->kvDataLen, kvDataLen) << "Length of KV data incorrect";
        EXPECT_EQ(memcmp(texture->kvData, kvData, kvDataLen), 0);
        if (texture)
            ktxTexture_Destroy(texture);
    }
}

TEST_F(ktxTexture_KVDataTest, SkipKVData) {
    ktxTexture* texture = 0;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT,
                                             &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->kvData == NULL) << "Raw KVData should not be loaded";
        ASSERT_TRUE(texture->kvDataHead == NULL) << "KVData should not be deserialized";
        if (texture)
            ktxTexture_Destroy(texture);
    }
}

/////////////////////////////////////////
// ktxTexture_IterateLoadLevelFaces tests
////////////////////////////////////////

TEST_F(ktxTexture1_IterateLoadLevelFacesTest, InvalidValueOnNullCallback) {
    ktxTexture* texture = 0;
    KTX_error_code result;
    ktxTexture1_IterateLoadLevelFacesTest* fixture = this;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             0, &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);

        EXPECT_EQ(ktxTexture_IterateLoadLevelFaces(texture, 0, fixture),
                  KTX_INVALID_VALUE);
        if (texture)
            ktxTexture_Destroy(texture);
    }
}

TEST_F(ktxTexture1_IterateLoadLevelFacesTest, InvalidOpWhenDataAlreadyLoaded) {
    ktxTexture* texture = 0;
    KTX_error_code result;
    ktxTexture1_IterateLoadLevelFacesTest* fixture = this;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                             &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";
        EXPECT_EQ(ktxTexture_IterateLoadLevelFaces(texture, iterCallback, fixture),
                  KTX_INVALID_OPERATION);
        if (texture)
            ktxTexture_Destroy(texture);
    }
}

TEST_F(ktxTexture1_IterateLoadLevelFacesTest, IterateImages) {
    ktxTexture* texture = 0;
    KTX_error_code result;
    ktxTexture1_IterateLoadLevelFacesTest* fixture = this;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             0, &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);

        EXPECT_EQ(ktxTexture_IterateLoadLevelFaces(texture, iterCallback, fixture),
                  KTX_SUCCESS);
        EXPECT_EQ(iterCbCalls, mipLevels)
                  << "No. of calls to iterCallback differs from number of mip levels";
        if (texture)
            ktxTexture_Destroy(texture);
    }
}

/////////////////////////////////////////
// ktxTexture_IterateLevelFaces tests
////////////////////////////////////////

TEST_F(ktxTexture1_IterateLevelFacesTest, InvalidValueOnNullCallback) {
    ktxTexture* texture = 0;
    KTX_error_code result;
    ktxTexture1_IterateLevelFacesTest* fixture = this;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                             &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";
        EXPECT_EQ(ktxTexture_IterateLevelFaces(texture, 0, fixture),
                  KTX_INVALID_VALUE);
        if (texture)
            ktxTexture_Destroy(texture);
    }
}

TEST_F(ktxTexture1_IterateLevelFacesTest, IterateImages) {
    ktxTexture* texture = 0;
    KTX_error_code result;
    ktxTexture1_IterateLevelFacesTest* fixture = this;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                             &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);

        EXPECT_EQ(ktxTexture_IterateLevelFaces(texture, iterCallback, fixture),
                  KTX_SUCCESS);
        EXPECT_EQ(iterCbCalls, mipLevels)
                  << "No. of calls to iterCallback differs from number of mip levels";
        if (texture)
            ktxTexture_Destroy(texture);
    }
}

TEST_F(ktxTexture2_IterateLevelFacesTest, InvalidValueOnNullCallback) {
    ktxTexture* texture;
    KTX_error_code result;
    ktxTexture2_IterateLevelFacesTest* fixture = this;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                             &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";
        EXPECT_EQ(ktxTexture_IterateLevelFaces(texture, 0, fixture),
                  KTX_INVALID_VALUE);
        if (texture)
            ktxTexture_Destroy(texture);
    }
}

TEST_F(ktxTexture2_IterateLevelFacesTest, IterateImages) {
    ktxTexture* texture;
    KTX_error_code result;
    ktxTexture2_IterateLevelFacesTest* fixture = this;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                             &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);

        EXPECT_EQ(ktxTexture_IterateLevelFaces(texture, iterCallback, fixture),
                  KTX_SUCCESS);
        EXPECT_EQ(iterCbCalls, mipLevels)
                  << "No. of calls to iterCallback differs from number of mip levels";
        if (texture)
            ktxTexture_Destroy(texture);
    }
}

/////////////////////////////////////////
// ktxTexture_IterateLevels tests
////////////////////////////////////////

TEST_F(ktxTexture2_IterateLevelsTest, InvalidValueOnNullCallback) {
    ktxTexture* texture;
    KTX_error_code result;
    ktxTexture2_IterateLevelsTest* fixture = this;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                             &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";
        EXPECT_EQ(ktxTexture_IterateLevels(texture, 0, fixture),
                  KTX_INVALID_VALUE);
        if (texture)
            ktxTexture_Destroy(texture);
    }
}

TEST_F(ktxTexture2_IterateLevelsTest, IterateLevels) {
    ktxTexture* texture = 0;
    KTX_error_code result;
    ktxTexture2_IterateLevelsTest* fixture = this;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                             &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);

        EXPECT_EQ(ktxTexture_IterateLevelFaces(texture, iterCallback, fixture),
                  KTX_SUCCESS);
        EXPECT_EQ(iterCbCalls, mipLevels)
                  << "No. of calls to iterCallback differs from number of mip levels";
        if (texture)
            ktxTexture_Destroy(texture);
    }
}

/////////////////////////////////////////
// ktxTexture_LoadImageData tests
////////////////////////////////////////

TEST_F(ktxTexture1_LoadImageDataTest, InvalidOpWhenDataAlreadyLoaded) {
    ktxTexture* texture = 0;
    KTX_error_code result;
    ktx_uint8_t* buf;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                             &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";
        buf = new ktx_uint8_t[paddedImageDataSize];
        EXPECT_EQ(ktxTexture_LoadImageData(texture, buf, paddedImageDataSize),
                  KTX_INVALID_OPERATION);
        if (texture)
            ktxTexture_Destroy(texture);
        delete[] buf;
    }
}

TEST_F(ktxTexture1_LoadImageDataTest, InvalidOpWhenDataAlreadyLoadedToExternal) {
    ktxTexture* texture = 0;
    KTX_error_code result;
    ktx_uint8_t* buf;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             0,
                                             &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData == NULL) << "Image data must not be loaded";
        buf = new ktx_uint8_t[paddedImageDataSize];
        EXPECT_EQ(ktxTexture_LoadImageData(texture, buf, paddedImageDataSize),
                  KTX_SUCCESS);
        EXPECT_EQ(ktxTexture_LoadImageData(texture, buf, paddedImageDataSize),
                  KTX_INVALID_OPERATION);
        if (texture)
            ktxTexture_Destroy(texture);
        delete[] buf;
    }
}

TEST_F(ktxTexture1_LoadImageDataTest, LoadImageDataInternal) {
    ktxTexture* texture = 0;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                             &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";
        EXPECT_EQ(paddedImageDataSize, ktxTexture_GetDataSize(texture));
        EXPECT_EQ(helper.compareTexture1Images(ktxTexture_GetData(texture)), true);
        if (texture)
            ktxTexture_Destroy(texture);
    }
}

TEST_F(ktxTexture1_LoadImageDataTest, LoadImageDataExternal) {
    ktxTexture* texture = 0;
    KTX_error_code result;
    ktx_uint8_t* buf;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             0,
                                             &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        buf = new ktx_uint8_t[paddedImageDataSize];
        EXPECT_EQ(ktxTexture_LoadImageData(texture, buf, paddedImageDataSize),
                  KTX_SUCCESS);
        EXPECT_EQ(paddedImageDataSize, ktxTexture_GetDataSize(texture));
        EXPECT_EQ(helper.compareTexture1Images(buf), true);
        if (texture)
            ktxTexture_Destroy(texture);
        delete[] buf;
    }
}

TEST_F(ktxTexture2_LoadImageDataTest, InvalidOpWhenDataAlreadyLoaded) {
    ktxTexture* texture = 0;
    KTX_error_code result;
    ktx_uint8_t* buf;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                             &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";
        buf = new ktx_uint8_t[paddedImageDataSize];
        EXPECT_EQ(ktxTexture_LoadImageData(texture, buf, paddedImageDataSize),
                  KTX_INVALID_OPERATION);
        if (texture)
            ktxTexture_Destroy(texture);
        delete[] buf;
    }
}

TEST_F(ktxTexture2_LoadImageDataTest, InvalidOpWhenDataAlreadyLoadedToExternal) {
    ktxTexture* texture = 0;
    KTX_error_code result;
    ktx_uint8_t* buf;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             0,
                                             &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData == NULL) << "Image data must not be loaded";
        buf = new ktx_uint8_t[paddedImageDataSize];
        EXPECT_EQ(ktxTexture_LoadImageData(texture, buf, paddedImageDataSize),
                  KTX_SUCCESS);
        EXPECT_EQ(ktxTexture_LoadImageData(texture, buf, paddedImageDataSize),
                  KTX_INVALID_OPERATION);
        if (texture)
            ktxTexture_Destroy(texture);
        delete[] buf;
    }
}

TEST_F(ktxTexture2_LoadImageDataTest, LoadImageDataInternal) {
    ktxTexture* texture = 0;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                             &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";
        EXPECT_EQ(paddedImageDataSize, ktxTexture_GetDataSize(texture));
        EXPECT_EQ(helper.compareTexture2Images(ktxTexture_GetData(texture)), true);
        if (texture)
            ktxTexture_Destroy(texture);
    }
}

TEST_F(ktxTexture2_LoadImageDataTest, LoadImageDataExternal) {
    ktxTexture* texture = 0;
    KTX_error_code result;
    ktx_uint8_t* buf;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             0,
                                             &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        buf = new ktx_uint8_t[paddedImageDataSize];
        EXPECT_EQ(ktxTexture_LoadImageData(texture, buf, paddedImageDataSize),
                  KTX_SUCCESS);
        EXPECT_EQ(paddedImageDataSize, ktxTexture_GetDataSize(texture));
        EXPECT_EQ(helper.compareTexture2Images(buf), true);
        if (texture)
            ktxTexture_Destroy(texture);
        delete[] buf;
    }
}

/////////////////////////////////////////////
// ktxTexture2_CreateCopyTest
////////////////////////////////////////////

TEST_F(ktxTexture2_CreateCopyTest, CreateCopy) {
    ktxTexture2* texture = 0;
    ktxTexture2* copyTexture = 0;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             0,
                                             (ktxTexture**)&texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        result = ktxTexture2_CreateCopy(texture, &copyTexture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(copyTexture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);

        EXPECT_EQ(compareTexture(copyTexture), true);
        EXPECT_EQ(memcmp(texture->pData, copyTexture->pData, texture->dataSize),
                  0);
        EXPECT_EQ(memcmp(texture->_protected, copyTexture->_protected,
                         sizeof(ktxTexture_protected)), 0);
        ktx_size_t privateSize = sizeof(ktxTexture2_private)
                               + sizeof(ktxLevelIndexEntry)
                               * (texture->numLevels - 1);
        EXPECT_EQ(memcmp(texture->_private, copyTexture->_private,
                         privateSize), 0);

        if (texture)
            ktxTexture_Destroy((ktxTexture*)texture);
        if (copyTexture)
            ktxTexture_Destroy((ktxTexture*)copyTexture);
    }
}

/////////////////////////////////////////////
// TestCreateInfo for size and offset tests.
////////////////////////////////////////////

class TestCreateInfo : public ktxTextureCreateInfo {
  public:
    TestCreateInfo() : TestCreateInfo(16) { }

    TestCreateInfo(ktx_uint32_t pixelSize)
    : TestCreateInfo(pixelSize, pixelSize, 1) { }

    TestCreateInfo(ktx_uint32_t width, ktx_uint32_t height, ktx_uint32_t depth)
    : TestCreateInfo(width, height, depth, 2, GL_RGBA8,
                     VK_FORMAT_R8G8B8A8_UNORM, KTX_FALSE, 1, 1) { }

    TestCreateInfo(ktx_uint32_t width, ktx_uint32_t height, ktx_uint32_t depth,
                   ktx_uint32_t dimensions, ktx_uint32_t internalformat,
                   ktx_uint32_t vkformat, ktx_bool_t array, ktx_uint32_t faces,
                   ktx_uint32_t layers) {
        baseWidth = width;
        baseHeight = height;
        baseDepth = depth;
        numDimensions = dimensions;
        generateMipmaps = KTX_FALSE;
        glInternalformat = internalformat;
        vkFormat = vkformat;
        isArray = array;
        numFaces = faces;
        numLayers = layers;
        numLevels = levelsFromSize(width, height, depth);
    };

    static ktx_uint32_t
    levelsFromSize(ktx_uint32_t width, ktx_uint32_t height, ktx_uint32_t depth) {
        ktx_uint32_t mipLevels;
        ktx_uint32_t max_dim = MAX(MAX(width, height), depth);
        for (mipLevels = 1; max_dim != 1; mipLevels++, max_dim >>= 1) { }
        return mipLevels;
    }
};

/////////////////////////////////////////
// ktxTexture_calcImageSize tests
////////////////////////////////////////

TEST(ktxTexture_calcImageSize, ImageSizeAtEachLevelRGBA2D) {
    ktxTexture* texture = 0;
    TestCreateInfo createInfo;
    KTX_error_code result;
    // Sizes for 16x16, 5 level RGBA8 texture.
    // level 0 ... level 4
    ktx_uint32_t ktx1sizes[] = {1024, 256, 64, 16, 4};
    ktx_uint32_t ktx2sizes[] = {1024, 256, 64, 16, 4};

    result = ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                                (ktxTexture1**)&texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture1_Create failed: "
                                 << ktxErrorString(result);
    for (ktx_uint32_t i = 0; i < createInfo.numLevels; i++) {
        ktx_size_t imageSize;
        imageSize = ktxTexture_calcImageSize(texture, i,
                                             KTX_FORMAT_VERSION_ONE);
        EXPECT_EQ(imageSize, ktx1sizes[i]);
        imageSize = ktxTexture_calcImageSize(texture, i,
                                             KTX_FORMAT_VERSION_TWO);
        EXPECT_EQ(imageSize, ktx2sizes[i]);
    }
    if (texture)
        ktxTexture_Destroy(texture);
}

TEST(ktxTexture_calcImageSize, ImageSizeAtEachLevelRGB2D) {
    ktxTexture* texture = 0;
    TestCreateInfo createInfo(9, 9, 1, 2, GL_RGB8,
                              VK_FORMAT_R8G8B8_UNORM, KTX_FALSE, 1, 1);
    KTX_error_code result;
    // Sizes for 9x9, 4 level RGB8 texture.
    // level 0 ... level 4
    ktx_uint32_t ktx1sizes[] = {28*9, 12*4, 8*2, 4*1};
    ktx_uint32_t ktx2sizes[] = {27*9, 12*4, 6*2, 3*1};

    result = ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                                (ktxTexture1**)&texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture1_Create failed: "
                                 << ktxErrorString(result);
    for (ktx_uint32_t i = 0; i < createInfo.numLevels; i++) {
        ktx_size_t imageSize;
        imageSize = ktxTexture_calcImageSize(texture, i,
                                             KTX_FORMAT_VERSION_ONE);
        EXPECT_EQ(imageSize, ktx1sizes[i]);
        imageSize = ktxTexture_calcImageSize(texture, i,
                                             KTX_FORMAT_VERSION_TWO);
        EXPECT_EQ(imageSize, ktx2sizes[i]);
    }
    if (texture)
        ktxTexture_Destroy(texture);
}

/////////////////////////////////////////
// ktxTexture_calcLevelSize tests
////////////////////////////////////////

TEST(ktxTexture_calcLevelSize, SizeOfEachLevelRGBA2D) {
    ktxTexture* texture = 0;
    TestCreateInfo createInfo;
    KTX_error_code result;
    // Sizes for 16x16, 5 level RGBA8 texture.
    // level 0 ... level 4
    ktx_uint32_t ktx1sizes[] = {1024, 256, 64, 16, 4};
    ktx_uint32_t ktx2sizes[] = {1024, 256, 64, 16, 4};

    result = ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                                (ktxTexture1**)&texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture1_Create failed: "
                                 << ktxErrorString(result);
    for (ktx_uint32_t i = 0; i < createInfo.numLevels; i++) {
        ktx_size_t levelSize;
        levelSize = ktxTexture_calcLevelSize(texture, i,
                                             KTX_FORMAT_VERSION_ONE);
        EXPECT_EQ(levelSize, ktx1sizes[i]);
        levelSize = ktxTexture_calcLevelSize(texture, i,
                                             KTX_FORMAT_VERSION_TWO);
        EXPECT_EQ(levelSize, ktx2sizes[i]);
    }
    if (texture)
        ktxTexture_Destroy(texture);
}

TEST(ktxTexture_calcLevelSize, SizeOfEachLevelRGB2D) {
    ktxTexture* texture = 0;
    TestCreateInfo createInfo(9, 9, 1, 2, GL_RGB8,
                              VK_FORMAT_R8G8B8_UNORM, KTX_FALSE, 1, 1);
    KTX_error_code result;
    // Sizes for 9x9, 4 level RGB8 texture.
    // level 0 ... level 4
    ktx_uint32_t ktx1sizes[] = {28*9, 12*4, 8*2, 4*1};
    ktx_uint32_t ktx2sizes[] = {27*9, 12*4, 6*2, 3*1};

    result = ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                                (ktxTexture1**)&texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture1_Create failed: "
                                 << ktxErrorString(result);
    for (ktx_uint32_t i = 0; i < createInfo.numLevels; i++) {
        ktx_size_t levelSize;
        levelSize = ktxTexture_calcLevelSize(texture, i,
                                             KTX_FORMAT_VERSION_ONE);
        EXPECT_EQ(levelSize, ktx1sizes[i]);
        levelSize = ktxTexture_calcLevelSize(texture, i,
                                             KTX_FORMAT_VERSION_TWO);
        EXPECT_EQ(levelSize, ktx2sizes[i]);
    }
    if (texture)
        ktxTexture_Destroy(texture);
}

/////////////////////////////////////////
// ktxTexture_calcLevelOffset tests
////////////////////////////////////////

TEST(ktxTexture_calcLevelOffset, OffsetOfEachLevelRGBA2D) {
    ktxTexture1* ktx1texture = 0;
    ktxTexture2* ktx2texture = 0;
    TestCreateInfo createInfo;
    KTX_error_code result;
    // Offsets for 16x16, 5 level RGBA8 texture.
    // KTX 1: level 0 ... level 4
    ktx_uint32_t ktx1offsets[] = {0, 1024, 1024+256, 1024+256+64, 1024+256+64+16};
    // KTX 2: level 0 ... level 4 with mip padding to a 4 byte alignment.
    ktx_uint32_t ktx2offsets[] = {4+16+64+256, 4+16+64, 4+16, 4, 0};

    result = ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                                &ktx1texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(ktx1texture != NULL) << "ktxTexture1_Create failed: "
                                 << ktxErrorString(result);
    result = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                                &ktx2texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(ktx2texture != NULL) << "ktxTexture2_Create failed: "
                                 << ktxErrorString(result);
    for (ktx_uint32_t i = 0; i < createInfo.numLevels; i++) {
        ktx_size_t levelOffset;
        levelOffset = ktxTexture1_calcLevelOffset(ktx1texture, i);
        EXPECT_EQ(levelOffset, ktx1offsets[i]);
        levelOffset = ktxTexture2_calcLevelOffset(ktx2texture, i);
        EXPECT_EQ(levelOffset, ktx2offsets[i]);
    }
    if (ktx1texture)
        ktxTexture_Destroy(ktxTexture(ktx1texture));
    if (ktx2texture)
        ktxTexture_Destroy(ktxTexture(ktx2texture));
}

TEST(ktxTexture_calcLevelOffset, OffsetOfEachLevelRGB2D) {
    ktxTexture1* ktx1texture = 0;
    ktxTexture2* ktx2texture = 0;
    TestCreateInfo createInfo(9, 9, 1, 2, GL_RGB8,
                              VK_FORMAT_R8G8B8_UNORM, KTX_FALSE, 1, 1);
    KTX_error_code result;
    // Offsets for 9x9, 4 level RGB8 texture.
    // KTX 1: level 0 ... level 4
    ktx_uint32_t ktx1offsets[] = {0, 28*9, 28*9+12*4, 28*9+12*4+8*2};
    // KTX 2: level 0 ... level 4 with mip padding to a 12 byte alignment.
    ktx_uint32_t ktx2offsets[] = {12*4+24, 6*2+12, 3*1+9, 0};

    result = ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                                &ktx1texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(ktx1texture != NULL) << "ktxTexture1_Create failed: "
                                 << ktxErrorString(result);
    result = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                                &ktx2texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(ktx2texture != NULL) << "ktxTexture2_Create failed: "
                                 << ktxErrorString(result);

    for (ktx_uint32_t i = 0; i < createInfo.numLevels; i++) {
        ktx_size_t levelOffset;
        levelOffset = ktxTexture1_calcLevelOffset(ktx1texture, i);
        EXPECT_EQ(levelOffset, ktx1offsets[i]);
        levelOffset = ktxTexture2_calcLevelOffset(ktx2texture, i);
        EXPECT_EQ(levelOffset, ktx2offsets[i]);
    }
    if (ktx1texture)
        ktxTexture_Destroy(ktxTexture(ktx1texture));
    if (ktx2texture)
        ktxTexture_Destroy(ktxTexture(ktx2texture));
}

TEST(ktxTexture_calcLevelOffset, OffsetOfEachLevelD16_UNORM_S8_UINT) {
    ktxTexture2* ktx2texture = 0;
    TestCreateInfo createInfo(9, 9, 1, 2, 0,
                              VK_FORMAT_D16_UNORM_S8_UINT, KTX_FALSE, 1, 1);
    KTX_error_code result;
    // Offsets for 9x9, 4 level  texture.
    // KTX 2: level 0 ... level 4 with mip padding to a 4 byte alignment.
    ktx_uint32_t ktx2offsets[] = {4+16+64, 4+16, 4, 0};

    result = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                                &ktx2texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(ktx2texture != NULL) << "ktxTexture2_Create failed: "
                                 << ktxErrorString(result);

    for (ktx_uint32_t i = 0; i < createInfo.numLevels; i++) {
        ktx_size_t levelOffset;
        levelOffset = ktxTexture2_calcLevelOffset(ktx2texture, i);
        EXPECT_EQ(levelOffset, ktx2offsets[i]);
    }
    if (ktx2texture)
        ktxTexture_Destroy(ktxTexture(ktx2texture));
}

TEST(ktxTexture_calcLevelOffset, OffsetOfEachLevelD32_SFLOAT_S8_UINT) {
    ktxTexture2* ktx2texture = 0;
    TestCreateInfo createInfo(9, 9, 1, 2, 0,
                              VK_FORMAT_D32_SFLOAT_S8_UINT, KTX_FALSE, 1, 1);
    KTX_error_code result;
    // Offsets for 9x9, 4 level  texture.
    // KTX 2: level 0 ... level 4 with mip padding to an 8 byte alignment.
    ktx_uint32_t ktx2offsets[] = {8+32+128, 8+32, 8, 0};

    result = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                                &ktx2texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(ktx2texture != NULL) << "ktxTexture2_Create failed: "
                                 << ktxErrorString(result);

    for (ktx_uint32_t i = 0; i < createInfo.numLevels; i++) {
        ktx_size_t levelOffset;
        levelOffset = ktxTexture2_calcLevelOffset(ktx2texture, i);
        EXPECT_EQ(levelOffset, ktx2offsets[i]);
    }
    if (ktx2texture)
        ktxTexture_Destroy(ktxTexture(ktx2texture));
}

/////////////////////////////////////////
// ktxTexture_GetImageOffset tests
////////////////////////////////////////

TEST(ktxTexture_GetImageOffsetTest, InvalidOpOnLevelFaceLayerTooBig) {
    ktxTexture* texture = 0;
    TestCreateInfo createInfo;
    KTX_error_code result;
    ktx_size_t offset;

    result = ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                                (ktxTexture1**)&texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture1_Create failed: "
                                 << ktxErrorString(result);
    EXPECT_EQ(ktxTexture_GetImageOffset(texture, createInfo.numLevels, 0, 0, &offset),
              KTX_INVALID_OPERATION);
    EXPECT_EQ(ktxTexture_GetImageOffset(texture, 0, createInfo.numLayers, 0, &offset),
              KTX_INVALID_OPERATION);
    EXPECT_EQ(ktxTexture_GetImageOffset(texture, 0, 0, createInfo.numFaces, &offset),
              KTX_INVALID_OPERATION);
    if (texture)
        ktxTexture_Destroy(texture);
}

TEST(ktxTexture_GetImageOffsetTest, ImageOffsetLevel) {
    //using createFlagBits = typename WriterTestHelper<GLubyte, 4, GL_RGBA8>::createFlagBits;
    TextureWriterTestHelper<GLubyte, 4, GL_RGBA8> helper;

    helper.resize(createFlagBits::eMipmapped, 1, 1, 2, 16, 16, 1);
    ktxTexture* texture = 0;
    KTX_error_code result;
    ktx_size_t expectedOffset, imageSize, offset;

    result = ktxTexture1_Create(&helper.createInfo,
                               KTX_TEXTURE_CREATE_NO_STORAGE,
                               (ktxTexture1**)&texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture1_Create failed: "
                                 << ktxErrorString(result);
    EXPECT_EQ(ktxTexture_GetImageOffset(texture, 0, 0, 0, &offset),
              KTX_SUCCESS);
    EXPECT_EQ(offset, 0U);
    // GL_RGBA8 is 1 x 4 bytes.
    imageSize = helper.createInfo.baseWidth
                * helper.createInfo.baseHeight * 1 * 4;
    expectedOffset = imageSize;
    EXPECT_EQ(ktxTexture_GetImageOffset(texture, 1, 0, 0, &offset),
              KTX_SUCCESS);
    EXPECT_EQ(offset, expectedOffset);
    // The image used to calculate imageDataSize has the same dimensions and
    // internalformat as those specified by createInfo.
    expectedOffset = helper.imageDataSize - 4;
    EXPECT_EQ(ktxTexture_GetImageOffset(texture, helper.createInfo.numLevels - 1, 0, 0, &offset),
              KTX_SUCCESS);
    EXPECT_EQ(offset, expectedOffset);
    if (texture)
        ktxTexture_Destroy(texture);
}

TEST(ktxTexture_GetImageOffsetTest, ImageOffsetWithRowPadding) {
    ktxTexture* texture = 0;
    TestCreateInfo createInfo;
    KTX_error_code result;
    ktx_size_t expectedOffset, imageSize, offset;
    ktx_uint32_t rowBytes, rowRounding;

    // Pick type and size that requires row padding for KTX_GL_UNPACK_ALIGNMENT.
    createInfo.glInternalformat = GL_RGB8;
    createInfo.baseWidth = 9;
    result = ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                                (ktxTexture1**)&texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture1_Create failed: "
                                 << ktxErrorString(result);
    rowBytes = 9 * 3;
    rowRounding = ROUNDING(rowBytes);
    imageSize = (rowBytes + rowRounding) * texture->baseHeight;
    expectedOffset = imageSize;
    EXPECT_EQ(ktxTexture_GetImageOffset(texture, 1, 0, 0, &offset),
              KTX_SUCCESS);
    EXPECT_EQ(offset, expectedOffset);

    expectedOffset = 0;
    for (ktx_uint32_t i = 0; i < texture->numLevels - 1; i++) {
        ktx_uint32_t levelWidth = MAX(1, texture->baseWidth >> i);
        ktx_uint32_t levelHeight = MAX(1, texture->baseHeight >> i);

        int levelRowBytes = levelWidth * 3;
        rowRounding = ROUNDING(levelRowBytes);
        levelRowBytes += rowRounding;
        imageSize = levelRowBytes * levelHeight;
        expectedOffset += imageSize;
    }
    EXPECT_EQ(ktxTexture_GetImageOffset(texture, createInfo.numLevels - 1, 0, 0, &offset),
              KTX_SUCCESS);
    EXPECT_EQ(offset, expectedOffset);
    if (texture)
        ktxTexture_Destroy(texture);
}

TEST(ktxTexture_GetImageOffsetTest, ImageOffsetArray) {
    ktxTexture* texture = 0;
    TestCreateInfo createInfo;
    KTX_error_code result;
    ktx_size_t expectedOffset, offset;
    ktx_uint32_t rowBytes, rowRounding, imageSize, layerSize;
    ktx_uint32_t levelWidth, levelHeight, levelImageSize, levelRowBytes;

    createInfo.glInternalformat = GL_RGB8;
    createInfo.baseWidth = 9;
    createInfo.numLayers = 3;
    result = ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                                (ktxTexture1**)&texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture1_Create failed: "
                                 << ktxErrorString(result);
    rowBytes = 9 * 3;
    rowRounding = ROUNDING(rowBytes);
    imageSize = (rowBytes + rowRounding) * createInfo.baseHeight;
    layerSize = imageSize * texture->numFaces;
    expectedOffset = layerSize * texture->numLayers;
    EXPECT_EQ(ktxTexture_GetImageOffset(texture, 1, 0, 0, &offset),
              KTX_SUCCESS);
    EXPECT_EQ(offset, expectedOffset);
    levelWidth = MAX(1, texture->baseWidth >> 1);
    levelHeight = MAX(1, texture->baseHeight >> 1);
    levelRowBytes = levelWidth * 3;
    rowRounding = ROUNDING(levelRowBytes);
    levelRowBytes += rowRounding;
    levelImageSize = levelRowBytes * levelHeight;
    expectedOffset += levelImageSize * 2;
    EXPECT_EQ(ktxTexture_GetImageOffset(texture, 1, 2, 0, &offset),
              KTX_SUCCESS);
    EXPECT_EQ(offset, expectedOffset);
    if (texture)
        ktxTexture_Destroy(texture);
}

TEST(ktxTexture_GetImageOffsetTest, ImageOffsetFace) {
    ktxTexture* texture = 0;
    TestCreateInfo createInfo;
    KTX_error_code result;
    ktx_size_t expectedOffset, offset;
    ktx_uint32_t rowBytes, rowRounding, imageSize, layerSize;
    ktx_uint32_t levelWidth, levelHeight, levelImageSize, levelRowBytes;

    createInfo.glInternalformat = GL_RGB8;
    createInfo.baseWidth = 9;
    createInfo.baseHeight = 9;
    createInfo.numLevels = 4;
    createInfo.numLayers = 1;
    createInfo.numFaces = 6;
    result = ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                                (ktxTexture1**)&texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture1_Create failed: "
                                 << ktxErrorString(result);
    rowBytes = 9 * 3;
    rowRounding = ROUNDING(rowBytes);
    imageSize = (rowBytes + rowRounding) * texture->baseHeight;
    layerSize = imageSize * texture->numFaces;
    expectedOffset = imageSize * 4;
    EXPECT_EQ(ktxTexture_GetImageOffset(texture, 0, 0, 4, &offset),
              KTX_SUCCESS);
    EXPECT_EQ(offset, expectedOffset);
    levelWidth = MAX(1, texture->baseWidth >> 1);
    levelHeight = MAX(1, texture->baseHeight >> 1);
    levelRowBytes = levelWidth * 3;
    rowRounding = ROUNDING(levelRowBytes);
    levelRowBytes += rowRounding;
    levelImageSize = levelRowBytes * levelHeight;
    expectedOffset = layerSize + levelImageSize * 3;
    EXPECT_EQ(ktxTexture_GetImageOffset(texture, 1, 0, 3, &offset),
              KTX_SUCCESS);
    EXPECT_EQ(offset, expectedOffset);
    if (texture)
        ktxTexture_Destroy(texture);
}

TEST(ktxTexture_GetImageOffsetTest, ImageOffsetArrayFace) {
    ktxTexture* texture = 0;
    TestCreateInfo createInfo;
    KTX_error_code result;
    ktx_size_t expectedOffset, offset;
    ktx_uint32_t rowBytes, rowRounding, imageSize, layerSize;
    ktx_uint32_t levelWidth, levelHeight, levelImageSize, levelRowBytes;

    createInfo.glInternalformat = GL_RGB8;
    createInfo.baseWidth = 9;
    createInfo.baseWidth = 9;
    createInfo.baseHeight = 9;
    createInfo.numLevels = 4;
    createInfo.numLayers = 3;
    createInfo.numFaces = 6;
    result = ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                                (ktxTexture1**)&texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture1_Create failed: "
                                 << ktxErrorString(result);
    rowBytes = 9 * 3;
    rowRounding = ROUNDING(rowBytes);
    imageSize = (rowBytes + rowRounding) * createInfo.baseHeight;
    layerSize = imageSize * texture->numFaces;
    expectedOffset = layerSize * createInfo.numLayers;
    EXPECT_EQ(ktxTexture_GetImageOffset(texture, 1, 0, 0, &offset),
              KTX_SUCCESS);
    EXPECT_EQ(offset, expectedOffset);
    levelWidth = MAX(1, createInfo.baseWidth >> 1);
    levelHeight = MAX(1, createInfo.baseHeight >> 1);
    levelRowBytes = levelWidth * 3;
    rowRounding = ROUNDING(levelRowBytes);
    levelRowBytes += rowRounding;
    levelImageSize = levelRowBytes * levelHeight;
    expectedOffset += levelImageSize * texture->numFaces * 2;
    EXPECT_EQ(ktxTexture_GetImageOffset(texture, 1, 2, 0, &offset),
              KTX_SUCCESS);
    EXPECT_EQ(offset, expectedOffset);
    expectedOffset += levelImageSize * 3; // 3 faces
    EXPECT_EQ(ktxTexture_GetImageOffset(texture, 1, 2, 3, &offset),
              KTX_SUCCESS);
    EXPECT_EQ(offset, expectedOffset);
    if (texture)
        ktxTexture_Destroy(texture);
}

/////////////////////////////////////////
// ktxTexture_Write tests
////////////////////////////////////////

TEST_F(ktxTexture1WriteTestRGB8, Write1D) {
    helper.resize(createFlagBits::eNone, 1, 1, 1, 32, 1, 1);
    runTest(false);
}

TEST_F(ktxTexture1WriteTestRGB8, Write1DNeedsPadding) {
    helper.resize(createFlagBits::eNone, 1, 1, 1, 9, 1, 1);
    runTest(false);
}

TEST_F(ktxTexture1WriteTestRGBA8, Write1DMipmap) {
    helper.resize(createFlagBits::eMipmapped, 1, 1, 1, 32, 1, 1);
    runTest(false);
}

TEST_F(ktxTexture1WriteTestRGB8, Write1DArray) {
    helper.resize(createFlagBits::eArray, 4, 1, 1, 32, 1, 1);
    runTest(false);
}

TEST_F(ktxTexture1WriteTestRGBA8, Write1DArrayMipmap) {
    helper.resize(createFlagBits::eMipmapped | createFlagBits::eArray,
                  4, 1, 1, 32, 1, 1);
    runTest(false);
}

TEST_F(ktxTexture1WriteTestRGB8, Write2D) {
    helper.resize(createFlagBits::eNone, 1, 1, 2, 32, 32, 1);
    runTest(true);
}

TEST_F(ktxTexture1WriteTestRGB8, Write2DMipmap) {
    helper.resize(createFlagBits::eMipmapped, 1, 1, 2, 32, 32, 1);
    runTest(true);
}

TEST_F(ktxTexture1WriteTestRGBA8, Write2DArray) {
    helper.resize(createFlagBits::eArray, 4, 1, 2, 32, 32, 1);
    runTest(true);
}

TEST_F(ktxTexture1WriteTestRGBA8, Write2DArrayMipmap) {
    helper.resize(createFlagBits::eArray | createFlagBits::eMipmapped,
                  4, 1, 2, 32, 32, 1);
    runTest(true);
}

TEST_F(ktxTexture1WriteTestRGB8, 3D) {
    helper.resize(createFlagBits::eNone,1, 1, 3, 32, 32, 32);
    runTest(true);
}

TEST_F(ktxTexture1WriteTestRGB8, Write3DMipmap) {
    helper.resize(createFlagBits::eMipmapped, 1, 1, 3, 8, 8, 2);
    runTest(true);
}

TEST_F(ktxTexture1WriteTestRGB8, WriteCubemap) {
    helper.resize(createFlagBits::eNone, 1, 6, 2, 32, 32, 1);
    runTest(true);
}

TEST_F(ktxTexture1WriteTestRGBA8, WriteCubemapMipmap) {
    helper.resize(createFlagBits::eMipmapped,
                  1, 6, 2, 32, 32, 1);
    runTest(true);
}
TEST_F(ktxTexture1WriteTestRGBA8, WriteCubemapArrayMipmap) {
    helper.resize(createFlagBits::eMipmapped | createFlagBits::eArray,
                  4, 6, 2, 32, 32, 1);
    runTest(true);
}

TEST_F(ktxTexture1WriteTestRG16, Write2DMipmap) {
    helper.resize(createFlagBits::eMipmapped, 1, 1, 2, 32, 32, 1);
    runTest(true);
}

//----------------------------------------------------------
// Template for base fixture for ktxTexture_WriteKTX2 tests.
//----------------------------------------------------------

#include "vkformat_enum.h"
#define LIBKTX // To make dfd.h not include vulkan/vulkan_core.h.
#include "dfdutils/dfd.h"

template<typename component_type, ktx_uint32_t numComponents,
         GLenum internalformat>
class ktxTexture1WriteKTX2TestBase
      : public ktxTexture1WriteTestBase<component_type, numComponents, internalformat> {
  public:
    using createFlags = typename WriterTestHelper<component_type, numComponents, internalformat>::createFlags;
    using ktxTexture1WriteTestBase<component_type, numComponents, internalformat>::helper;

    ktxTexture1WriteKTX2TestBase() {
        requiredLevelAlignment = lcm4(sizeof(component_type) * numComponents);
    }

    void runTest(bool writeOrientationMeta, bool writeWriterMeta = true) {
        ktxTexture1* texture = 0;
        KTX_error_code result;
        ktx_uint8_t* ktxMemFile;
        ktx_size_t ktxMemFileLen;
        ktx_uint8_t* filePtr;
        ktxHashList* hl;
        ktxHashList_Create(&hl);
        ktx_uint8_t* kvData;
        ktx_uint32_t kvDataLen;

        result = ktxTexture1_Create(&helper.createInfo,
                                   KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                   &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture1_Create failed: "
                                     << ktxErrorString(result);

        if (writeOrientationMeta) {
            // Reminder: this is for the KTX 1 texture we have just created.
            ktxHashList_AddKVPair(&texture->kvDataHead, KTX_ORIENTATION_KEY,
                                  (unsigned int)strlen(helper.orientation) + 1,
                                  helper.orientation);
            // This is for the comparison metadata.
            ktxHashList_AddKVPair(hl, KTX_ORIENTATION_KEY,
                                  (unsigned int)strlen(helper.orientation_ktx2) + 1,
                                  helper.orientation_ktx2);
        }
        // N.B. Writer metadata is not legal in a KTX v1 file but we know
        // we're going to write this out as a v2 file so okay.
        if (writeWriterMeta) {
            ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
                                  (uint32_t)helper.writer_ktx2.size(),
                                  helper.writer_ktx2.data());
            ktxHashList_AddKVPair(hl, KTX_WRITER_KEY,
                                  (uint32_t)helper.writer_ktx2.size(),
                                  helper.writer_ktx2.data());
        }
        // Now update the comparison metadata by doing the things WriteKTX2 is
        // supposed to do so we can check it's actually doing it..
        ktxHashListEntry* pWriter = nullptr;;
        ktxHashList_FindEntry(hl, KTX_WRITER_KEY, &pWriter);
        result = appendLibId(hl, pWriter);
        EXPECT_EQ(result, KTX_SUCCESS);
        ktxHashList_Sort(hl);
        // And retrieve the comparison metadata.
        ktxHashList_Serialize(hl, &kvDataLen, &kvData);

        result = helper.copyImagesToTexture(texture);
        EXPECT_EQ(result, KTX_SUCCESS);

        EXPECT_EQ(helper.compareTexture1Images(texture->pData), true);

        result = ktxTexture1_WriteKTX2ToMemory(texture,
                                               &ktxMemFile,
                                               &ktxMemFileLen);

        ASSERT_TRUE(result == KTX_SUCCESS) << "ktxTexture_WriteKTX2ToMemory failed: "
                                           << ktxErrorString(result);

        KTX_header2* header = (KTX_header2*)ktxMemFile;

        EXPECT_EQ(memcmp(header, ktxId2, sizeof(ktxId2)), 0);
        EXPECT_EQ(helper.texinfo.compare(header), true);

        // Check the format descriptor.
        // This uses the same code to generate the comparator DFD as the
        // code under test. However we have separate tests for the
        // generator, so can be reasonably confident in it. This test
        // ensures there is a DFD in the file.
        ktx_uint32_t* dfd = vk2dfd(static_cast<VkFormat>(header->vkFormat));
        EXPECT_EQ(memcmp(ktxMemFile + header->dataFormatDescriptor.byteOffset,
                         dfd,
                         *dfd), 0);

        // Check the metadata.
        filePtr = ktxMemFile + header->keyValueData.byteOffset;
        EXPECT_EQ(header->keyValueData.byteLength, kvDataLen);
        EXPECT_EQ(memcmp(filePtr, kvData, kvDataLen), 0);
        filePtr += kvDataLen;

#if 0
        if (writeOrientationMeta) {
            EXPECT_EQ(header->keyValueData.byteLength,
                      helper.kvDataLenAll_ktx2);
            EXPECT_EQ(memcmp(filePtr, helper.kvDataAll_ktx2,
                             helper.kvDataLenAll_ktx2), 0);
            filePtr += helper.kvDataLenAll_ktx2;
        } else {
            EXPECT_EQ(header->keyValueData.byteLength,
                      helper.kvDataLenWriter_ktx2);
            EXPECT_EQ(memcmp(filePtr, helper.kvDataWriter_ktx2,
                             helper.kvDataLenWriter_ktx2), 0);
            filePtr += helper.kvDataLenWriter_ktx2;
        }
#endif
        // Offset of level 0 is first item in leveIndex after header.
        ktxLevelIndexEntry* levelIndex =
            reinterpret_cast<ktxLevelIndexEntry*>(ktxMemFile + sizeof(*header));

        ktx_uint64_t prevOffset = UINT64_MAX;
        for (ktx_uint32_t level = 0; level < helper.numLevels; level++) {
            ktx_uint64_t levelOffset = levelIndex[level].byteOffset;
            // Check offset is properly aligned.
            EXPECT_EQ(levelOffset % requiredLevelAlignment, 0U);
            // Check mipmaps are in order of increasing size in the file
            // therefore each offset should be smaller than the previous.
            EXPECT_LE(levelOffset, prevOffset);
            prevOffset = levelOffset;
        }

        EXPECT_EQ(helper.compareRawImages(levelIndex, ktxMemFile), true);
        delete ktxMemFile;
        ktxTexture_Destroy(ktxTexture(texture));
    }

    // Test rejection of unrecognized keys and passing of proprietary keys.
    void runTest(const char* unrecognizedKey, const char* proprietaryKey) {
        ktxTexture1* texture = 0;
        KTX_error_code result;
        ktx_uint8_t* ktxMemFile;
        ktx_size_t ktxMemFileLen;
        ktx_uint8_t* filePtr;
        ktx_uint8_t* kvData;
        ktx_uint32_t kvDataLen;

        result = ktxTexture1_Create(&helper.createInfo,
                                   KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                   &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture1_Create failed: "
                                     << ktxErrorString(result);

        ktxHashList* hl;
        ktxHashList_Create(&hl);
        ktxHashList* hlists[2] = {&texture->kvDataHead, hl};
        // Add desired keys & values to both the texture and a comparator.
        char rubbishValue[] = "some rubbish value";
        for (uint32_t i = 0; i < 2; i++) {
            // N.B. Writer metadata is not legal in a KTX v1 file but we know
            // we're going to write this out as a v2 file so okay.
            ktxHashList_AddKVPair(hlists[i], KTX_WRITER_KEY,
                                  (uint32_t)helper.writer_ktx2.size(),
                                  helper.writer_ktx2.data());
            if (unrecognizedKey) {
                ktxHashList_AddKVPair(hlists[i], unrecognizedKey,
                                      sizeof(rubbishValue),
                                      rubbishValue);
            }
            if (proprietaryKey) {
                ktxHashList_AddKVPair(hlists[i], proprietaryKey,
                                      sizeof(rubbishValue),
                                      rubbishValue);
            }
            ktxHashList_Sort(hlists[i]);
        }
        // Get the library to add its Id to the writer key so it will be
        // included in the serialized data.
        ktxHashListEntry* pWriter;
        ktxHashList_FindEntry(hl, KTX_WRITER_KEY, &pWriter);
        appendLibId(hl, pWriter);
        ktxHashList_Sort(hl);
        ktxHashList_Serialize(hl, &kvDataLen, &kvData);
        ktxHashList_Destruct(hl);

        result = helper.copyImagesToTexture(texture);
        EXPECT_EQ(result, KTX_SUCCESS);

        EXPECT_EQ(helper.compareTexture1Images(texture->pData), true);

        result = ktxTexture1_WriteKTX2ToMemory(texture,
                                               &ktxMemFile,
                                               &ktxMemFileLen);

        if (unrecognizedKey == NULL) {
            ASSERT_TRUE(result == KTX_SUCCESS) << "ktxTexture_WriteKTX2ToMemory failed: "
                                               << ktxErrorString(result);

            KTX_header2* header = (KTX_header2*)ktxMemFile;

            EXPECT_EQ(memcmp(header, ktxId2, sizeof(ktxId2)), 0);
            EXPECT_EQ(helper.texinfo.compare(header), true);

            // Check the format descriptor.
            ktx_uint32_t* dfd = vk2dfd(static_cast<VkFormat>(header->vkFormat));
            EXPECT_EQ(memcmp(ktxMemFile + header->dataFormatDescriptor.byteOffset,
                             dfd,
                             *dfd), 0);

            // Check the metadata.
            filePtr = ktxMemFile + header->keyValueData.byteOffset;
            EXPECT_EQ(header->keyValueData.byteLength, kvDataLen);
            EXPECT_EQ(memcmp(filePtr, kvData, kvDataLen), 0);
            filePtr += helper.kvDataLen;

            // Offset of level 0 is first item in leveIndex after header.
            ktxLevelIndexEntry* levelIndex =
                reinterpret_cast<ktxLevelIndexEntry*>(ktxMemFile + sizeof(*header));

            ktx_uint64_t offset = UINT64_MAX;
            for (ktx_uint32_t level = 0; level < helper.numLevels; level++) {
                ktx_uint64_t levelOffset = levelIndex[level].byteOffset;
                // Check offset is properly aligned.
                EXPECT_EQ(levelOffset % requiredLevelAlignment, 0U);
                // Check mipmaps are in order of increasing size in the file
                // therefore each offset should be smaller than the previous.
                EXPECT_LE(levelOffset, offset);
                offset = levelOffset;
            }

            EXPECT_EQ(helper.compareRawImages(levelIndex, ktxMemFile), true);

            delete ktxMemFile;
        } else {
            EXPECT_EQ(result, KTX_INVALID_OPERATION);
        }

        ktxTexture_Destroy(ktxTexture(texture));
        delete kvData;

    }
  protected:
    ktx_uint32_t requiredLevelAlignment;
};

class ktxTexture1WriteKTX2TestRGBA8: public ktxTexture1WriteKTX2TestBase<GLubyte, 4, GL_RGBA8> { };
class ktxTexture1WriteKTX2TestRGB8: public ktxTexture1WriteKTX2TestBase<GLubyte, 3, GL_RGB8> { };
class ktxTexture1WriteKTX2TestRG16: public ktxTexture1WriteKTX2TestBase<GLushort, 2, GL_RG16> { };

/////////////////////////////////////////
// ktxTexture_WriteKTX2 tests
////////////////////////////////////////

TEST_F(ktxTexture1WriteKTX2TestRGBA8, Write1DNoOrientationMetadata) {
    helper.resize(createFlagBits::eNone, 1, 1, 1, 32, 1, 1);
    runTest(false);
}

TEST_F(ktxTexture1WriteKTX2TestRGBA8, Write1DNoWriterMetadata) {
    helper.resize(createFlagBits::eNone, 1, 1, 1, 32, 1, 1);
    runTest(false, false);
}

TEST_F(ktxTexture1WriteKTX2TestRGBA8, Write1DMipmap) {
    helper.resize(createFlagBits::eMipmapped, 1, 1, 1, 32, 1, 1);
    runTest(false);
}


TEST_F(ktxTexture1WriteKTX2TestRGB8, Write1DArray) {
    helper.resize(createFlagBits::eArray, 4, 1, 1, 32, 1, 1);
    runTest(false);
}

TEST_F(ktxTexture1WriteKTX2TestRGBA8, Write1DArrayMipmap) {
    helper.resize(createFlagBits::eMipmapped | createFlagBits::eArray,
                  4, 1, 1, 32, 1, 1);
    runTest(false);
}

TEST_F(ktxTexture1WriteKTX2TestRGBA8, Write2DNoOrientationMetadata) {
    helper.resize(createFlagBits::eNone, 1, 1, 2, 32, 32, 1);
    runTest(false);
}

TEST_F(ktxTexture1WriteKTX2TestRGBA8, Write2DNoWriterMetadata) {
    helper.resize(createFlagBits::eNone, 1, 1, 2, 32, 32, 1);
    runTest(false, false);
}

TEST_F(ktxTexture1WriteKTX2TestRGB8, Write2DMipmap) {
    helper.resize(createFlagBits::eMipmapped, 1, 1, 2, 32, 32, 1);
    runTest(true);
}

TEST_F(ktxTexture1WriteKTX2TestRGB8, Write2DMipmapUnrecognizedMetadata1) {
    helper.resize(createFlagBits::eMipmapped, 1, 1, 2, 32, 32, 1);
    runTest("KTXOrientation", NULL);
}

TEST_F(ktxTexture1WriteKTX2TestRGB8, Write2DMipmapUnrecognizedMetadata2) {
    helper.resize(createFlagBits::eMipmapped, 1, 1, 2, 32, 32, 1);
    runTest("ktxOrientation", NULL);
}

TEST_F(ktxTexture1WriteKTX2TestRGB8, Write2DMipmapProprietaryMetadata) {
    helper.resize(createFlagBits::eMipmapped, 1, 1, 2, 32, 32, 1);
    runTest(NULL, "MyProprietaryKey");
}

TEST_F(ktxTexture1WriteKTX2TestRGB8, Write2DMipmapUnrecogAndPropMetadata) {
    helper.resize(createFlagBits::eMipmapped, 1, 1, 2, 32, 32, 1);
    runTest("KTXOrientation", "MyProprietaryKey");
}
TEST_F(ktxTexture1WriteKTX2TestRGBA8, Write2DArray) {
    helper.resize(createFlagBits::eArray, 4, 1, 2, 32, 32, 1);
    runTest(true);
}

TEST_F(ktxTexture1WriteKTX2TestRGBA8, Write2DArrayMipmap) {
    helper.resize(createFlagBits::eArray | createFlagBits::eMipmapped,
                  4, 1, 2, 32, 32, 1);
    runTest(true);
}

TEST_F(ktxTexture1WriteKTX2TestRGB8, 3D) {
    helper.resize(createFlagBits::eNone,1, 1, 3, 32, 32, 32);
    runTest(true);
}

TEST_F(ktxTexture1WriteKTX2TestRGB8, Write3DMipmap) {
    helper.resize(createFlagBits::eMipmapped, 1, 1, 3, 8, 8, 2);
    runTest(true);
}

TEST_F(ktxTexture1WriteKTX2TestRGB8, WriteCubemap) {
    helper.resize(createFlagBits::eNone, 1, 6, 2, 32, 32, 1);
    runTest(true);
}

TEST_F(ktxTexture1WriteKTX2TestRGBA8, WriteCubemapMipmap) {
    helper.resize(createFlagBits::eMipmapped,
                  1, 6, 2, 32, 32, 1);
    runTest(true);
}

TEST_F(ktxTexture1WriteKTX2TestRGBA8, WriteCubemapArrayMipmap) {
    helper.resize(createFlagBits::eMipmapped | createFlagBits::eArray,
                  4, 6, 2, 32, 32, 1);
    runTest(true);
}

TEST_F(ktxTexture1WriteKTX2TestRG16, Write2DMipmap) {
    helper.resize(createFlagBits::eMipmapped, 1, 1, 2, 32, 32, 1);
    runTest(true);
}

//----------------------------------------------------------
// Template for base fixture for ktxTexture2_Read tests.
//----------------------------------------------------------

template<typename component_type, ktx_uint32_t numComponents,
         GLenum internalformat>
class ktxTexture2ReadTestBase
      : public ktxTexture1WriteTestBase<component_type, numComponents, internalformat> {
  public:
    using createFlags = typename WriterTestHelper<component_type, numComponents, internalformat>::createFlags;
    using ktxTexture1WriteTestBase<component_type, numComponents, internalformat>::helper;

    ktxTexture2ReadTestBase() { }

    ~ktxTexture2ReadTestBase() {
        if (ktx2MemFile) delete ktx2MemFile;
    }

    void resize(createFlags flags,
                ktx_uint32_t numLayers, ktx_uint32_t numFaces,
                ktx_uint32_t numDimensions,
                ktx_uint32_t width, ktx_uint32_t height, ktx_uint32_t depth)
    {
        ktxTexture1* texture = 0;
        KTX_error_code result;

        helper.resize(flags, numLayers, numFaces, numDimensions,
                      width, height, depth);

        result = ktxTexture1_Create(&helper.createInfo,
                                    KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                    &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture1_Create failed: "
                                     << ktxErrorString(result);

        // Reminder: this is for the KTX 1 texture we have just created.
        ktxHashList_AddKVPair(&texture->kvDataHead, KTX_ORIENTATION_KEY,
                              (unsigned int)strlen(helper.orientation) + 1,
                              helper.orientation);
        ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
                              (uint32_t)helper.writer_ktx2.size(),
                              helper.writer_ktx2.data());

        result = helper.copyImagesToTexture(texture);
        EXPECT_EQ(result, KTX_SUCCESS);

        EXPECT_EQ(helper.compareTexture1Images(texture->pData), true);

        result = ktxTexture1_WriteKTX2ToMemory(texture,
                                               &ktx2MemFile,
                                               &ktx2MemFileLen);
        ASSERT_TRUE(result == KTX_SUCCESS) << "ktxTexture1_WriteKTX2ToMemory failed: "
                                           << ktxErrorString(result);
        fileHeader = (KTX_header2*)ktx2MemFile;
        levelIndex = (ktxLevelIndexEntry*)(ktx2MemFile + sizeof(KTX_header2));

        ktxTexture1_destruct(texture);
    }

    void runTest() {
        ktxTexture2* texture2 = 0;
        KTX_error_code result;

        result = ktxTexture2_CreateFromMemory(ktx2MemFile, ktx2MemFileLen,
                                        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                        &texture2);

        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture2 != NULL) << "ktxTexture2_Create failed: "
                                      << ktxErrorString(result);

        EXPECT_EQ(texture2->classId, ktxTexture2_c);
        EXPECT_EQ(helper.texinfo.compare(texture2), true);
        EXPECT_NE(texture2->kvDataHead, (ktxKVListEntry*)0);

        // Check the levelOffsets are as expected.
        ktx_size_t baseOffset = levelIndex[helper.numLevels - 1].byteOffset;
        for (ktx_uint32_t level = 0; level < texture2->numLevels; level++) {
            ktx_size_t levelOffset;
            result = ktxTexture2_GetImageOffset(texture2, level, 0, 0,
                                                &levelOffset);
            EXPECT_EQ(result, KTX_SUCCESS);
            EXPECT_EQ(levelOffset, levelIndex[level].byteOffset - baseOffset);
        }

        ktxTexture2_destruct(texture2);

    }

    protected:
        ktx_uint8_t* ktx2MemFile;
        ktx_size_t ktx2MemFileLen;
        KTX_header2* fileHeader;
        ktxLevelIndexEntry* levelIndex;
};

class ktxTexture2ReadTestRGBA8: public ktxTexture2ReadTestBase<GLubyte, 4, GL_RGBA8> { };

/////////////////////////////////////////
// ktxTexture_WriteKTX2 tests
////////////////////////////////////////

TEST_F(ktxTexture2ReadTestRGBA8, Read1D) {
    resize(createFlagBits::eNone, 1, 1, 1, 32, 1, 1);
    runTest();
}

TEST_F(ktxTexture2ReadTestRGBA8, Read2D) {
    resize(createFlagBits::eNone, 1, 1, 2, 32, 32, 1);
    runTest();
}

TEST_F(ktxTexture2ReadTestRGBA8, Read3D) {
    resize(createFlagBits::eNone, 1, 1, 3, 32, 32, 32);
    runTest();
}

TEST_F(ktxTexture2ReadTestRGBA8, Read1DMipmap) {
    resize(createFlagBits::eMipmapped, 1, 1, 1, 64, 1, 1);
    runTest();
}

TEST_F(ktxTexture2ReadTestRGBA8, Read2DMipmap) {
    resize(createFlagBits::eMipmapped, 1, 1, 2, 64, 64, 1);
    runTest();
}

TEST_F(ktxTexture2ReadTestRGBA8, Read3DMipmap) {
    resize(createFlagBits::eMipmapped, 1, 1, 3, 64, 64, 32);
    runTest();
}

class ktxTexture2_BasisCompressTest : public ktxTexture2TestBase<GLubyte, 4, GL_RGBA8>  { };

/////////////////////////////////////////
// ktxTexture2_BasisCompress tests
////////////////////////////////////////

TEST_F(ktxTexture2_BasisCompressTest, Compress) {
    ktxTexture2* texture;
    ktx_uint64_t dataSize;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture2_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                              KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";

        dataSize = texture->dataSize;
        result = ktxTexture2_CompressBasis(texture, 0);
        EXPECT_EQ(result, KTX_SUCCESS);
        EXPECT_EQ(texture->supercompressionScheme, KTX_SS_BASIS_LZ);
        EXPECT_TRUE(texture->_private->_supercompressionGlobalData > (ktx_uint8_t*)0);
        EXPECT_EQ(texture->numLevels, helper.numLevels);
        EXPECT_LT(texture->dataSize, dataSize);
        // How else to test the result?

        result = ktxTexture2_TranscodeBasis(texture, KTX_TTF_BC1_RGB, 0);
        EXPECT_EQ(result, KTX_SUCCESS);
        if (texture)
            ktxTexture_Destroy(ktxTexture(texture));
    }
}

class ktxTexture2_GetNumComponentsTestR8 : public ktxTexture2TestBase<GLubyte, 1, GL_R8> { };
class ktxTexture2_GetNumComponentsTestRG8 : public ktxTexture2TestBase<GLubyte, 2, GL_RG8> { };
class ktxTexture2_GetNumComponentsTestRGB8 : public ktxTexture2TestBase<GLubyte, 3, GL_RGB8> { };
class ktxTexture2_GetNumComponentsTestRGBA8 : public ktxTexture2TestBase<GLubyte, 4, GL_RGBA8> { };
class ktxTexture2_MetadataTest : public ktxTexture2TestBase<GLubyte, 4, GL_RGBA8> { };

////////////////////////////////////////////
// ktxTexture2_GetNumComponents tests
///////////////////////////////////////////

TEST_F(ktxTexture2_GetNumComponentsTestR8, Uncompressed) {
    ktxTexture2* texture;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture2_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                              KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";

        ktx_uint32_t components = ktxTexture2_GetNumComponents(texture);
        EXPECT_EQ(components, 1U);
        if (texture)
            ktxTexture_Destroy(ktxTexture(texture));
    }
}

TEST_F(ktxTexture2_GetNumComponentsTestR8, BasisLZ) {
    ktxTexture2* texture;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture2_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                              KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";

        ktx_uint32_t components = ktxTexture2_GetNumComponents(texture);
        EXPECT_EQ(components, 1U);
        ktxTexture2_CompressBasis(texture, 0);
        EXPECT_EQ(components, ktxTexture2_GetNumComponents(texture));
        if (texture)
            ktxTexture_Destroy(ktxTexture(texture));
    }
}

TEST_F(ktxTexture2_GetNumComponentsTestR8, UASTC) {
    ktxTexture2* texture;
    KTX_error_code result;
    ktxBasisParams cparams = { };

    if (ktxMemFile != NULL) {
        result = ktxTexture2_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                              KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";

        ktx_uint32_t components = ktxTexture2_GetNumComponents(texture);
        EXPECT_EQ(components, 1U);
        cparams.uastc = KTX_TRUE;
        ktxTexture2_CompressBasisEx(texture, &cparams);
        EXPECT_EQ(components, ktxTexture2_GetNumComponents(texture));
        if (texture)
            ktxTexture_Destroy(ktxTexture(texture));
    }
}

TEST_F(ktxTexture2_GetNumComponentsTestRG8, Uncompressed) {
    ktxTexture2* texture;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture2_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                              KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";

        ktx_uint32_t components = ktxTexture2_GetNumComponents(texture);
        EXPECT_EQ(components, 2U);
        if (texture)
            ktxTexture_Destroy(ktxTexture(texture));
    }
}

TEST_F(ktxTexture2_GetNumComponentsTestRG8, BasisLZ) {
    ktxTexture2* texture;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture2_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                              KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";

        ktx_uint32_t components = ktxTexture2_GetNumComponents(texture);
        EXPECT_EQ(components, 2U);
        ktxTexture2_CompressBasis(texture, 0);
        EXPECT_EQ(components, ktxTexture2_GetNumComponents(texture));
    }
}

TEST_F(ktxTexture2_GetNumComponentsTestRG8, UASTC) {
    ktxTexture2* texture;
    KTX_error_code result;
    ktxBasisParams cparams = { };

    if (ktxMemFile != NULL) {
        result = ktxTexture2_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                              KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";

        ktx_uint32_t components = ktxTexture2_GetNumComponents(texture);
        EXPECT_EQ(components, 2U);
        cparams.uastc = KTX_TRUE;
        ktxTexture2_CompressBasisEx(texture, &cparams);
        EXPECT_EQ(components, ktxTexture2_GetNumComponents(texture));
        if (texture)
            ktxTexture_Destroy(ktxTexture(texture));
    }
}

TEST_F(ktxTexture2_GetNumComponentsTestRGB8, Uncompressed) {
    ktxTexture2* texture;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture2_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                              KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";

        ktx_uint32_t components = ktxTexture2_GetNumComponents(texture);
        EXPECT_EQ(components, 3U);
        if (texture)
            ktxTexture_Destroy(ktxTexture(texture));
    }
}

TEST_F(ktxTexture2_GetNumComponentsTestRGB8, BasisLZ) {
    ktxTexture2* texture;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture2_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                              KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";

        ktx_uint32_t components = ktxTexture2_GetNumComponents(texture);
        EXPECT_EQ(components, 3U);
        ktxTexture2_CompressBasis(texture, 0);
        EXPECT_EQ(components, ktxTexture2_GetNumComponents(texture));
        if (texture)
            ktxTexture_Destroy(ktxTexture(texture));
    }
}

TEST_F(ktxTexture2_GetNumComponentsTestRGB8, UASTC) {
    ktxTexture2* texture;
    KTX_error_code result;
    ktxBasisParams cparams = { };

    if (ktxMemFile != NULL) {
        result = ktxTexture2_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                              KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";

        ktx_uint32_t components = ktxTexture2_GetNumComponents(texture);
        EXPECT_EQ(components, 3U);
        cparams.uastc = KTX_TRUE;
        ktxTexture2_CompressBasisEx(texture, &cparams);
        EXPECT_EQ(components, ktxTexture2_GetNumComponents(texture));
        if (texture)
            ktxTexture_Destroy(ktxTexture(texture));
    }
}

TEST_F(ktxTexture2_GetNumComponentsTestRGBA8, Uncompressed) {
    ktxTexture2* texture;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture2_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                              KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";

        ktx_uint32_t components = ktxTexture2_GetNumComponents(texture);
        EXPECT_EQ(components, 4U);
        if (texture)
            ktxTexture_Destroy(ktxTexture(texture));
    }
}

TEST_F(ktxTexture2_GetNumComponentsTestRGBA8, BasisLZ) {
    ktxTexture2* texture;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture2_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                              KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";

        ktx_uint32_t components = ktxTexture2_GetNumComponents(texture);
        EXPECT_EQ(components, 4U);
        ktxTexture2_CompressBasis(texture, 0);
        EXPECT_EQ(components, ktxTexture2_GetNumComponents(texture));
        if (texture)
            ktxTexture_Destroy(ktxTexture(texture));
    }
}

TEST_F(ktxTexture2_GetNumComponentsTestRGBA8, UASTC) {
    ktxTexture2* texture;
    KTX_error_code result;
    ktxBasisParams cparams = { };

    if (ktxMemFile != NULL) {
        result = ktxTexture2_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                              KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";

        ktx_uint32_t components = ktxTexture2_GetNumComponents(texture);
        EXPECT_EQ(components, 4U);
        cparams.uastc = KTX_TRUE;
        ktxTexture2_CompressBasisEx(texture, &cparams);
        EXPECT_EQ(components, ktxTexture2_GetNumComponents(texture));
        if (texture)
            ktxTexture_Destroy(ktxTexture(texture));
    }
}

TEST_F(ktxTexture2_MetadataTest, EmptyValue) {
    ktxTexture2* texture;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture2_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                              KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image storage not allocated";

        result = ktxHashList_AddKVPair(&texture->kvDataHead,
                                       "MSCtestKey", 0, nullptr);
        EXPECT_EQ(result, KTX_SUCCESS);

        ktx_size_t newMemFileLen;
        ktx_uint8_t* newMemFile;
        result = ktxTexture_WriteToMemory(ktxTexture(texture), &newMemFile,
                                          &newMemFileLen);
        EXPECT_EQ(result, KTX_SUCCESS);
        if (texture)
            ktxTexture_Destroy(ktxTexture(texture));

        result = ktxTexture2_CreateFromMemory(newMemFile, newMemFileLen,
                                              KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image storage not allocated";

        ktx_uint32_t valueLen;
        ktx_uint8_t* value;
        result = ktxHashList_FindValue(&texture->kvDataHead,
                                      "MSCtestKey", &valueLen, (void**)&value);
        EXPECT_EQ(result, KTX_SUCCESS);
        EXPECT_EQ(valueLen, 0U);
        EXPECT_EQ(value, nullptr);

        if (newMemFile)
            free(newMemFile);
        if (texture)
            ktxTexture_Destroy(ktxTexture(texture));
    }
}

#if defined(TestNoMetadata)
TEST_F(ktxTexture2_MetadataTest, NoMetadata) {
    ktxTexture2* texture;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture2_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                              KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image storage not allocated";

        ktxHashList_Destruct(&texture->kvDataHead);
        ktxTexture(texture)->kvDataHead = nullptr;
        ktxTexture(texture)->kvDataLen = 0;


        ktx_size_t newMemFileLen;
        ktx_uint8_t* newMemFile;
        ::__disableWriterMetadata__ = KTX_TRUE;
        result = ktxTexture_WriteToMemory(ktxTexture(texture), &newMemFile,
                                          &newMemFileLen);
        ::__disableWriterMetadata__ = KTX_FALSE;
        EXPECT_EQ(result, KTX_SUCCESS);
        if (texture)
            ktxTexture_Destroy(ktxTexture(texture));

        result = ktxTexture2_CreateFromMemory(newMemFile, newMemFileLen,
                                              KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image storage not allocated";

        ktx_uint32_t valueLen;
        ktx_uint8_t* value;
        EXPECT_EQ(result, KTX_SUCCESS);
        EXPECT_EQ(texture->kvDataLen, 0U);
        EXPECT_EQ(texture->kvDataHead, nullptr);

        if (newMemFile)
            free(newMemFile);
        if (texture)
            ktxTexture_Destroy(ktxTexture(texture));
    }
}
#endif

TEST_F(ktxTexture2_MetadataTest, NoLibVersionDupOnMultipleWrites) {
    ktxTexture2* texture;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture2_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                              KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image storage not allocated";

        const ktx_uint32_t iterations = 2;
        ktx_size_t newMemFileLens[iterations];
        ktx_uint8_t* newMemFiles[iterations];
        for (uint32_t i = 0; i < iterations; i++) {
            result = ktxTexture_WriteToMemory(ktxTexture(texture),
                                              &newMemFiles[i],
                                              &newMemFileLens[i]);
            EXPECT_EQ(result, KTX_SUCCESS);
        }
        for (uint32_t i = 1; i < iterations; i++) {
            EXPECT_EQ(newMemFileLens[i-1], newMemFileLens[i]);
        }

        if (texture)
            ktxTexture_Destroy(ktxTexture(texture));

        std::string writers[iterations];
        for (uint32_t i = 0; i < iterations; i++) {
            ktx_uint32_t valueLen;
            ktx_uint8_t* value;
            result = ktxTexture2_CreateFromMemory(newMemFiles[i],
                                                  newMemFileLens[i],
                                                  KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                                  &texture);
            ASSERT_TRUE(result == KTX_SUCCESS);
            ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                         << ktxErrorString(result);
            ASSERT_TRUE(texture->pData != NULL) << "Image storage not allocated";

            result = ktxHashList_FindValue(&texture->kvDataHead,
                                          "KTXwriter",
                                          &valueLen, (void**)&value);
            EXPECT_EQ(result, KTX_SUCCESS);
            // We want ktxWriteTo* to NUL terminate the value when adding
            // the libktx version.
            ASSERT_TRUE(value[valueLen-1] == '\0')
                        << "KTXwriter not NUL terminated";
            writers[i] = (char*)value;
            if (texture)
                ktxTexture_Destroy(ktxTexture(texture));
        }

        for (uint32_t i = 1; i < iterations; i++) {
            // This is a valid test because we know all our calls to libktx
            // use the same version of libktx.
            EXPECT_EQ(0, writers[i-1].compare(writers[i]));
        }

        for (uint32_t i = 0; i < iterations; i++) {
            if (newMemFiles[i])
                free(newMemFiles[i]);
        }
    }
}

TEST_F(ktxTexture2_MetadataTest, LibVersionUpdatedCorrectly) {
    ktxTexture2* texture;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture2_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                              KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image storage not allocated";

        ktx_uint32_t curWriterLen;
        ktx_uint8_t* curWriterVal;
        result = ktxHashList_FindValue(&texture->kvDataHead,
                                       "KTXwriter",
                                        &curWriterLen, (void**)&curWriterVal);
        EXPECT_EQ(result, KTX_SUCCESS);
        // We want ktxWriteTo* to NUL terminate the value when adding
        // the libktx version.
        ASSERT_TRUE(curWriterVal[curWriterLen-1] == '\0')
                    << "KTXwriter not NUL terminated";
        // The pointer returned by FindValue becomes invalid when the texture
        // is destroyed hence saving to this string. -1 to omit the terminator.
        std::string curWriter((char*)curWriterVal, curWriterLen-1);
        std::string writer(curWriter);
        size_t slash_pos = writer.find_last_of('/');
        ASSERT_TRUE(slash_pos != std::string::npos)
                    << "KTXwriter does not have lib version.";
        writer.replace(slash_pos + 2, std::string::npos, "libktx v3.0.0");
        result = ktxHashList_AddKVPair(&texture->kvDataHead,
                                       "KTXwriter",
                                       (ktx_uint32_t)writer.length(),
                                       writer.c_str());
        EXPECT_EQ(result, KTX_SUCCESS);


        ktx_size_t newMemFileLen;
        ktx_uint8_t* newMemFile;
        result = ktxTexture_WriteToMemory(ktxTexture(texture),
                                          &newMemFile,
                                          &newMemFileLen);
        EXPECT_EQ(result, KTX_SUCCESS);

        if (texture)
            ktxTexture_Destroy(ktxTexture(texture));

        ktx_uint32_t newWriterLen;
        ktx_uint8_t* newWriterVal;
        result = ktxTexture2_CreateFromMemory(newMemFile,
                                              newMemFileLen,
                                              KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                              &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image storage not allocated";

        result = ktxHashList_FindValue(&texture->kvDataHead,
                                       "KTXwriter",
                                        &newWriterLen, (void**)&newWriterVal);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(newWriterVal[newWriterLen-1] == '\0')
                    << "KTXwriter not NUL terminated";

        EXPECT_EQ(0, curWriter.compare((char *)newWriterVal));

        if (texture)
            ktxTexture_Destroy(ktxTexture(texture));

        if (newMemFile)
            free(newMemFile);
    }
}

////////////////////////////////////////////
// Unicode file name tests
///////////////////////////////////////////

#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#define OS_SEP '\\'
#define UNIX_SEP '/'
#else
#define OS_SEP '/'
#endif

std::string imagePath;

std::string combinePaths(std::string const a, std::string const b) {
    if (a.back() == OS_SEP) {
        return a + b;
#if defined(_WIN32)
    }
    else if (a.back() == UNIX_SEP) {
        return a + b;
#endif
    }
    else {
        return a + OS_SEP + b;
    }
}

TEST(UnicodeFileNames, CreateFrom) {
    std::vector<std::string> fileSet = {
        "hűtő.ktx",
        "hűtő.ktx2",
        "نَسِيج.ktx",
        "نَسِيج.ktx2",
        "テクスチャ.ktx",
        "テクスチャ.ktx2",
        "质地.ktx",
        "质地.ktx2",
        "조직.ktx",
        "조직.ktx2"
    };

    std::vector<std::string>::const_iterator it;

    for (it = fileSet.begin(); it < fileSet.end(); it++) {
        ktx_error_code_e result;
        ktxTexture* texture;

        std::string path = combinePaths(imagePath, *it);
        result = ktxTexture_CreateFromNamedFile(
            path.c_str(),
            KTX_TEXTURE_CREATE_NO_FLAGS,
            &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        EXPECT_NE(texture, (ktxTexture*)0);
        ktxTexture_Destroy(texture);

        size_t dotIndex = path.find_last_of('.');
        if (path.substr(dotIndex + 1).compare("ktx") == 0) {
            result = ktxTexture1_CreateFromNamedFile(
                path.c_str(),
                KTX_TEXTURE_CREATE_NO_FLAGS,
                (ktxTexture1**)&texture);
        } else {
            result = ktxTexture2_CreateFromNamedFile(
                path.c_str(),
                KTX_TEXTURE_CREATE_NO_FLAGS,
                (ktxTexture2**)&texture);
        }
        EXPECT_EQ(result, KTX_SUCCESS);
        EXPECT_NE(texture, (ktxTexture*)0);
        ktxTexture_Destroy(texture);
    }
}

}  // namespace

GTEST_API_ int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);

    if (!::testing::FLAGS_gtest_list_tests) {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " <test images path>\n";
            return -1;
        }

        imagePath = std::string(argv[1]);

        struct stat info;

        if (stat(imagePath.data(), &info) != 0) {
            std::cerr << "Cannot access " << imagePath << std::endl;
            return -2;
        }
        else if (!(info.st_mode & S_IFDIR)) {
            std::cerr << imagePath << "is not a valid directory\n";
            return -3;
        }
    }

    return RUN_ALL_TESTS();
}

