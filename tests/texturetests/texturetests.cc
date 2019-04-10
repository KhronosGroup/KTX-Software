/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

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
 * ©2010-2018 Mark Callow, <khronos at callow dot im>.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined(_WIN32)
  #define _CRT_SECURE_NO_WARNINGS
  #if _MSC_VER < 1900
    #define snprintf _snprintf
  #endif
#endif

#include <string.h>
#include "GL/glcorearb.h"
#include "ktx.h"
#include "ktxint.h"
#include "gtest/gtest.h"
#include "wthelper.h"

#define ROUNDING(x) \
        (3 - ((x + KTX_GL_UNPACK_ALIGNMENT-1) % KTX_GL_UNPACK_ALIGNMENT));

namespace {

//-------------------------------------------------------
// Helper for base fixture & ktxTexture_WriterTest cases.
//-------------------------------------------------------

template<typename component_type, ktx_uint32_t numComponents,
         GLenum internalformat>
class TextureWriterTestHelper
    : public WriterTestHelper<component_type, numComponents, internalformat> {
  public:
    typedef typename WriterTestHelper<component_type, numComponents, internalformat>::createFlags createFlags;
    typedef typename WriterTestHelper<component_type, numComponents, internalformat>::createFlagBits createFlagBits;
    using WriterTestHelper<component_type, numComponents, internalformat>::images;
    using WriterTestHelper<component_type, numComponents, internalformat>::imageList;
    using WriterTestHelper<component_type, numComponents, internalformat>::width;
    using WriterTestHelper<component_type, numComponents, internalformat>::height;
    using WriterTestHelper<component_type, numComponents, internalformat>::levelsFromSize;

	TextureWriterTestHelper() {}

    void
    resize(createFlags flags, ktx_uint32_t numLayers, ktx_uint32_t numFaces,
           ktx_uint32_t numDimensions,
           ktx_uint32_t width, ktx_uint32_t height, ktx_uint32_t depth)
    {
        WriterTestHelper<component_type, numComponents, internalformat>::resize(
                                               flags, numLayers, numFaces,
                                               numDimensions, width,
                                               height, depth);
        createInfo.resize(flags, numLayers, numFaces,
                          numDimensions, width, height, depth);
    }

    // Compare images as loaded into a ktxTexture object with our image.
    bool
    compareTextureImages(ktx_uint8_t* pData)
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

    KTX_error_code
    copyImagesToTexture(ktxTexture* texture) {
        KTX_error_code result;

        for (ktx_uint32_t level = 0; level < images.size(); level++) {
            for (ktx_uint32_t layer = 0; layer < images[level].size(); layer++) {
                for (ktx_uint32_t faceSlice = 0; faceSlice < images[level][layer].size(); faceSlice++) {
                    ktx_size_t imageBytes = images[level][layer][faceSlice].size() * sizeof(component_type);
                    ktx_uint8_t* imageDataPtr = (ktx_uint8_t*)(images[level][layer][faceSlice].data());
                    result = ktxTexture_SetImageFromMemory(texture,
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
                    ktx_uint32_t numLayers, ktx_uint32_t numFaces,
                    ktx_uint32_t numDimensions, ktx_uint32_t width,
                    ktx_uint32_t height, ktx_uint32_t depth)
        {
            baseWidth = width;
            baseHeight = height;
            baseDepth = depth;
            this->numDimensions = numDimensions;
            generateMipmaps = flags & createFlagBits::eGenerateMipmaps
                              ? KTX_TRUE : KTX_FALSE;
            isArray = flags & createFlagBits::eArray ? KTX_TRUE : KTX_FALSE;
            this->numFaces = numFaces;
            this->numLayers = numLayers;
            numLevels = flags & createFlagBits::eMipmapped
                        ? levelsFromSize(width, height, depth) : 1;
        };
    } createInfo;
    
};

const ktx_uint8_t ktxId[12] = KTX_IDENTIFIER_REF;

///////////////////////////////////////////////////////////
// Test fixtures
///////////////////////////////////////////////////////////

//----------------------------------------------------
// Base fixture for ktxTexture and related test cases.
//----------------------------------------------------

typedef TextureWriterTestHelper<GLubyte, 4, GL_RGBA8>::createFlagBits createFlagBits;

class ktxTextureTestBase : public ::testing::Test {
  protected:
    ktxTextureTestBase() : pixelSize(16)
    {
        helper.resize(createFlagBits::eMipmapped, 1, 1, 2, 16, 16, 1);
        // Create a KTX file in memory for testing.
        
        KTX_error_code errorCode;
        
        ktxMemFile = 0;
        iterCbCalls = 0;

        mipLevels = helper.numLevels;
        
        // Create the in-memory KTX file

        errorCode = ktxWriteKTXM(&ktxMemFile, &ktxMemFileLen,
                                 &texInfo, kvDataLen, kvData,
                                 mipLevels, &images.front());
       if (KTX_SUCCESS != errorCode) {
            ADD_FAILURE() << "ktxWriteKTXM failed: "
                          << ktxErrorString(errorCode);
       }
    }
    
    ~ktxTextureTestBase() {
        if (ktxMemFile != NULL) delete ktxMemFile;
    }
    
    KTX_error_code KTXAPIENTRY
    iterCallback(int miplevel, int face,
                 int width, int height, int depth,
                 ktx_uint32_t faceLodSize,
                 void* pixels)
    {
        int expectedWidth = pixelSize >> miplevel;
        EXPECT_EQ(width, expectedWidth);
        EXPECT_EQ(faceLodSize, expectedWidth * expectedWidth * 4);
        EXPECT_EQ(memcmp(pixels, images[miplevel].data, images[miplevel].size),
                  0);
        iterCbCalls++;
        return KTX_SUCCESS;
    }
    
    bool
    compareTexture(ktxTexture* texture)
    {
        if (texture->glInternalformat != texInfo.glInternalFormat)
            return false;
        if (texture->glBaseInternalformat != texInfo.glBaseInternalFormat)
            return false;
        if (texture->glFormat != texInfo.glFormat)
            return false;
        if (texture->glType != texInfo.glType)
            return false;
        if (ktxTexture_glTypeSize(texture) != texInfo.glTypeSize)
            return false;
        if (texture->baseWidth != texInfo.pixelWidth)
            return false;
        if (texInfo.pixelHeight == 0) {
            if (texture->baseHeight != 1)
                return false;
        } else if (texture->baseHeight != texInfo.pixelHeight)
            return false;
        if (texInfo.pixelDepth == 0) {
            if (texture->baseDepth != 1)
                return false;
        } else if (texture->baseDepth != texInfo.pixelDepth)
            return false;
        if (texture->numFaces != texInfo.numberOfFaces)
            return false;
        if (texture->numLevels != texInfo.numberOfMipmapLevels)
            return false;
        return true;
    }
    
    static KTX_error_code
    iterCallback(int miplevel, int face,
                  int width, int height,
                  int depth,
                  ktx_uint32_t faceLodSize,
                  void* pixels, void* userdata)
    {
        ktxTextureTestBase* fixture = (ktxTextureTestBase*)userdata;
        return fixture->iterCallback(miplevel, face, width, height, depth,
                                     faceLodSize, pixels);
    }

    TextureWriterTestHelper<GLubyte, 4, GL_RGBA8> helper;
    KTX_texture_info& texInfo = helper.texinfo;
    ktxTextureCreateInfo& createInfo = helper.createInfo;
    unsigned char*& kvData = helper.kvData;
    unsigned int& kvDataLen = helper.kvDataLen;
    
    unsigned char* ktxMemFile;
    GLsizei ktxMemFileLen;
    const int pixelSize;
    int mipLevels;
    unsigned int iterCbCalls;

    ktx_size_t& imageDataSize = helper.imageDataSize;
    std::vector< std::vector < std::vector < std::vector<GLubyte>  > > >& imageData = helper.images;
    std::vector<KTX_image_info>& images = helper.imageList;
};

//----------------------------------------------------
// Template for base fixture for ktxTextureWrite tests.
//----------------------------------------------------

template<typename component_type, ktx_uint32_t numComponents,
         GLenum internalformat>
class ktxTextureWriteTestBase : public ::testing::Test {
  public:
    using createFlags = typename WriterTestHelper<component_type, numComponents, internalformat>::createFlags;
    ktxTextureWriteTestBase() { }

    void runTest(bool writeMetadata) {
        ktxTexture* texture;
        KTX_error_code result;
        ktx_uint8_t* ktxMemFile;
        ktx_size_t ktxMemFileLen;
        ktx_uint8_t* filePtr;

        result = ktxTexture_Create(&helper.createInfo,
                                   KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                   &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_Create failed: "
                                     << ktxErrorString(result);

        if (writeMetadata)
            ktxHashList_AddKVPair(&texture->kvDataHead, KTX_ORIENTATION_KEY,
                                  (unsigned int)strlen(helper.orientation) + 1,
                                  helper.orientation);

        result = helper.copyImagesToTexture(texture);
        ASSERT_TRUE(result == KTX_SUCCESS);

        EXPECT_EQ(helper.compareTextureImages(texture->pData), true);
        result = ktxTexture_WriteToMemory(texture, &ktxMemFile, &ktxMemFileLen);
     
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
        EXPECT_EQ(helper.compareRawImages(filePtr), true);

        delete ktxMemFile;
        ktxTexture_Destroy(texture);
    }

    TextureWriterTestHelper<component_type, numComponents, internalformat> helper;
};

//---------------------------
// Actual test fixtures
//---------------------------

class ktxTexture_CreateTest : public ktxTextureTestBase { };
class ktxTexture_KVDataTest : public ktxTextureTestBase { };
class ktxTexture_IterateLoadLevelFacesTest : public ktxTextureTestBase { };
class ktxTexture_IterateLevelFacesTest : public ktxTextureTestBase { };
class ktxTexture_LoadImageDataTest : public ktxTextureTestBase { };

class ktxTextureWriteTestRGBA8 : public ktxTextureWriteTestBase<GLubyte, 4, GL_RGBA8> { };
class ktxTextureWriteTestRGB8 : public ktxTextureWriteTestBase<GLubyte, 3, GL_RGB8> { };
class ktxTextureWriteTestRG16 : public ktxTextureWriteTestBase<GLshort, 2, GL_RG16> { };
//using createFlagBits = typename WriterTestHelper<GLubyte, 4, GL_RGBA8>::createFlagBits;

/////////////////////////////////////////
// ktxTexture_Create tests
////////////////////////////////////////

TEST_F(ktxTexture_CreateTest, InvalidValueOnNullParams) {
    ktxTexture* texture;

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
    ktxTexture* texture;
    KTX_error_code result;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             0, &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        EXPECT_EQ(compareTexture(texture), true);
        EXPECT_EQ(texture->isCompressed, KTX_FALSE);
        EXPECT_EQ(texture->generateMipmaps, KTX_FALSE);
        EXPECT_EQ(texture->numDimensions, 2);
        EXPECT_EQ(texture->numLayers, 1);
        EXPECT_EQ(texture->isArray, KTX_FALSE);
        if (texture)
            ktxTexture_Destroy(texture);
    }
}
    
TEST_F(ktxTexture_CreateTest, CreateEmpty) {
    ktxTexture* texture;
    KTX_error_code result;
    
    result = ktxTexture_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                               &texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture_Create failed: "
                                 << ktxErrorString(result);
    if (texture)
        ktxTexture_Destroy(texture);
}
    
TEST_F(ktxTexture_CreateTest, InvalidValueTooManyMipLevels) {
    ktxTexture* texture;

    createInfo.numLevels += 1;

    EXPECT_EQ(ktxTexture_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE, &texture),
              KTX_INVALID_OPERATION);
}

TEST_F(ktxTexture_CreateTest, InvalidOpOnSetImagesNoStorage) {
    ktxTexture* texture;
    KTX_error_code result;
    
    result = ktxTexture_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                               &texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture_Create failed: "
                                 << ktxErrorString(result);
    
    // Type RGBA UNSIGNED_BYTE -> *4
    ktx_size_t imageBytes = imageData[0].size() * 4;
    ktx_uint8_t* imageDataPtr = (ktx_uint8_t*)(&imageData[0].front());
    // Allocate the image data and initialize it to a color.
    EXPECT_EQ(ktxTexture_SetImageFromMemory(texture, 0, 0, 0,
                                            imageDataPtr,
                                            imageBytes),
              KTX_INVALID_OPERATION);
    ASSERT_TRUE(result == KTX_SUCCESS);
    
    if (texture)
        ktxTexture_Destroy(texture);
}
    
TEST_F(ktxTexture_CreateTest, CreateEmptyAndSetImages) {
    ktxTexture* texture;
    KTX_error_code result;
    
    result = ktxTexture_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                               &texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture_Create failed: "
                                 << ktxErrorString(result);

    result = helper.copyImagesToTexture(texture);
    ASSERT_TRUE(result == KTX_SUCCESS);
    // imageData is an RGBA texture so no rounding is necessary and we can
    // use this simple comparison.
    EXPECT_EQ(helper.compareTextureImages(texture->pData), true);

    if (texture)
        ktxTexture_Destroy(texture);
}
    
TEST_F(ktxTexture_CreateTest, CreateEmptySetImagesWriteToMemory) {
    ktxTexture* texture;
    KTX_error_code result;
    ktx_uint8_t* testMemFile;
    ktx_size_t testMemFileLen;
    char orientation[10];

    result = ktxTexture_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                               &texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture_Create failed: "
                                 << ktxErrorString(result);

    snprintf(orientation, sizeof(orientation), KTX_ORIENTATION2_FMT,
             'r', 'd');
    ktxHashList_AddKVPair(&texture->kvDataHead, KTX_ORIENTATION_KEY,
                          (unsigned int)strlen(orientation) + 1,
                          orientation);
    result = helper.copyImagesToTexture(texture);
    ASSERT_TRUE(result == KTX_SUCCESS);
    EXPECT_EQ(helper.compareTextureImages(texture->pData), true);
    EXPECT_EQ(ktxTexture_WriteToMemory(texture, &testMemFile, &testMemFileLen),
              KTX_SUCCESS);
    EXPECT_EQ(testMemFileLen, ktxMemFileLen);
    EXPECT_EQ(memcmp(testMemFile, ktxMemFile, ktxMemFileLen), 0);
    
    if (texture)
        ktxTexture_Destroy(texture);
}
    
/////////////////////////////////////////
// ktxTexture_KVData tests
////////////////////////////////////////
    
TEST_F(ktxTexture_KVDataTest, KVDataDeserialized) {
    ktxTexture* texture;
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
    ktxTexture* texture;
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
    ktxTexture* texture;
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
    
TEST_F(ktxTexture_IterateLoadLevelFacesTest, InvalidValueOnNullCallback) {
    ktxTexture* texture;
    KTX_error_code result;
    ktxTexture_IterateLoadLevelFacesTest* fixture = this;
    
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

TEST_F(ktxTexture_IterateLoadLevelFacesTest, InvalidOpWhenDataAlreadyLoaded) {
    ktxTexture* texture;
    KTX_error_code result;
    ktxTexture_IterateLoadLevelFacesTest* fixture = this;

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

TEST_F(ktxTexture_IterateLoadLevelFacesTest, IterateImages) {
    ktxTexture* texture;
    KTX_error_code result;
    ktxTexture_IterateLoadLevelFacesTest* fixture = this;
    
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
// ktxTexture_IterateLoadLevelFaces tests
////////////////////////////////////////

TEST_F(ktxTexture_IterateLevelFacesTest, InvalidValueOnNullCallback) {
    ktxTexture* texture;
    KTX_error_code result;
    ktxTexture_IterateLevelFacesTest* fixture = this;
    
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

TEST_F(ktxTexture_IterateLevelFacesTest, IterateImages) {
    ktxTexture* texture;
    KTX_error_code result;
    ktxTexture_IterateLevelFacesTest* fixture = this;
    
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
    
TEST_F(ktxTexture_LoadImageDataTest, InvalidOpWhenDataAlreadyLoaded) {
    ktxTexture* texture;
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
        buf = new ktx_uint8_t[imageDataSize];
        EXPECT_EQ(ktxTexture_LoadImageData(texture, buf, imageDataSize),
                  KTX_INVALID_OPERATION);
        if (texture)
            ktxTexture_Destroy(texture);
        delete buf;
    }
}

TEST_F(ktxTexture_LoadImageDataTest, InvalidOpWhenDataAlreadyLoadedToExternal) {
    ktxTexture* texture;
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
        buf = new ktx_uint8_t[imageDataSize];
        EXPECT_EQ(ktxTexture_LoadImageData(texture, buf, imageDataSize),
                  KTX_SUCCESS);
        EXPECT_EQ(ktxTexture_LoadImageData(texture, buf, imageDataSize),
                  KTX_INVALID_OPERATION);
        if (texture)
            ktxTexture_Destroy(texture);
        delete buf;
    }
}

TEST_F(ktxTexture_LoadImageDataTest, LoadImageDataInternal) {
    ktxTexture* texture;
    KTX_error_code result;
    
    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                             &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";
        EXPECT_EQ(imageDataSize, ktxTexture_GetSize(texture));
        EXPECT_EQ(helper.compareTextureImages(ktxTexture_GetData(texture)), true);
        if (texture)
            ktxTexture_Destroy(texture);
    }
}

TEST_F(ktxTexture_LoadImageDataTest, LoadImageDataExternal) {
    ktxTexture* texture;
    KTX_error_code result;
    ktx_uint8_t* buf;

    if (ktxMemFile != NULL) {
        result = ktxTexture_CreateFromMemory(ktxMemFile, ktxMemFileLen,
                                             0,
                                             &texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        buf = new ktx_uint8_t[imageDataSize];
        EXPECT_EQ(ktxTexture_LoadImageData(texture, buf, imageDataSize),
                  KTX_SUCCESS);
        EXPECT_EQ(imageDataSize, ktxTexture_GetSize(texture));
        EXPECT_EQ(helper.compareTextureImages(buf), true);
        if (texture)
            ktxTexture_Destroy(texture);
        delete buf;
    }
}
    
/////////////////////////////////////////
// ktxTexture_GetImageOffset tests
////////////////////////////////////////

class TestCreateInfo : public ktxTextureCreateInfo {
  public:
    TestCreateInfo() : TestCreateInfo(16) { }

    TestCreateInfo(ktx_uint32_t pixelSize)
    : TestCreateInfo(pixelSize, pixelSize, 1) { }

    TestCreateInfo(ktx_uint32_t width, ktx_uint32_t height, ktx_uint32_t depth) {
        baseWidth = width;
        baseHeight = height;
        baseDepth = depth;
        numDimensions = 2;
        generateMipmaps = KTX_FALSE;
        glInternalformat = GL_RGBA8;
        isArray = KTX_FALSE;
        numFaces = 1;
        numLayers = 1;
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

TEST(ktxTexture_GetImageOffsetTest, InvalidOpOnLevelFaceLayerTooBig) {
    ktxTexture* texture;
    TestCreateInfo createInfo;
    KTX_error_code result;
    ktx_size_t offset;

    result = ktxTexture_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                               &texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture_Create failed: "
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
    ktxTexture* texture;
    KTX_error_code result;
    ktx_size_t expectedOffset, imageSize, offset;
    
    result = ktxTexture_Create(&helper.createInfo,
                               KTX_TEXTURE_CREATE_NO_STORAGE,
                               &texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture_Create failed: "
                                 << ktxErrorString(result);
    EXPECT_EQ(ktxTexture_GetImageOffset(texture, 0, 0, 0, &offset),
              KTX_SUCCESS);
    EXPECT_EQ(offset, 0);
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
    ktxTexture* texture;
    TestCreateInfo createInfo;
    KTX_error_code result;
    ktx_size_t expectedOffset, imageSize, offset;
    ktx_uint32_t rowBytes, rowRounding;
    
    // Pick type and size that requires row padding for KTX_GL_UNPACK_ALIGNMENT.
    createInfo.glInternalformat = GL_RGB8;
    createInfo.baseWidth = 9;
    result = ktxTexture_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                               &texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture_Create failed: "
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
    ktxTexture* texture;
    TestCreateInfo createInfo;
    KTX_error_code result;
    ktx_size_t expectedOffset, offset;
    ktx_uint32_t rowBytes, rowRounding, imageSize, layerSize;
    ktx_uint32_t levelWidth, levelHeight, levelImageSize, levelRowBytes;

    createInfo.glInternalformat = GL_RGB8;
    createInfo.baseWidth = 9;
    createInfo.numLayers = 3;
    result = ktxTexture_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                               &texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture_Create failed: "
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
    ktxTexture* texture;
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
    result = ktxTexture_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                               &texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture_Create failed: "
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
    ktxTexture* texture;
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
    result = ktxTexture_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                               &texture);
    EXPECT_EQ(result, KTX_SUCCESS);
    ASSERT_TRUE(texture != NULL) << "ktxTexture_Create failed: "
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

TEST_F(ktxTextureWriteTestRGB8, Write1D) {
    helper.resize(createFlagBits::eNone, 1, 1, 1, 32, 1, 1);
    runTest(false);
}

TEST_F(ktxTextureWriteTestRGB8, Write1DNeedsPadding) {
    helper.resize(createFlagBits::eNone, 1, 1, 1, 9, 1, 1);
    runTest(false);
}

TEST_F(ktxTextureWriteTestRGBA8, Write1DMipmap) {
    helper.resize(createFlagBits::eMipmapped, 1, 1, 1, 32, 1, 1);
    runTest(false);
}
TEST_F(ktxTextureWriteTestRGB8, Write1DArray) {
    helper.resize(createFlagBits::eArray, 4, 1, 1, 32, 1, 1);
    runTest(false);
}

TEST_F(ktxTextureWriteTestRGBA8, Write1DArrayMipmap) {
    helper.resize(createFlagBits::eMipmapped | createFlagBits::eArray,
                  4, 1, 1, 32, 1, 1);
    runTest(false);
}

TEST_F(ktxTextureWriteTestRGB8, Write2D) {
    helper.resize(createFlagBits::eNone, 1, 1, 2, 32, 32, 1);
    runTest(true);
}

TEST_F(ktxTextureWriteTestRGB8, Write2DMipmap) {
    helper.resize(createFlagBits::eMipmapped, 1, 1, 2, 32, 32, 1);
    runTest(true);
}

TEST_F(ktxTextureWriteTestRGBA8, Write2DArray) {
    helper.resize(createFlagBits::eArray, 4, 1, 2, 32, 32, 1);
    runTest(true);
}

TEST_F(ktxTextureWriteTestRGBA8, Write2DArrayMipmap) {
    helper.resize(createFlagBits::eArray | createFlagBits::eMipmapped,
                  4, 1, 2, 32, 32, 1);
    runTest(true);
}

TEST_F(ktxTextureWriteTestRGB8, 3D) {
    helper.resize(createFlagBits::eNone,1, 1, 3, 32, 32, 32);
    runTest(true);
}

TEST_F(ktxTextureWriteTestRGB8, Write3DMipmap) {
    helper.resize(createFlagBits::eMipmapped, 1, 1, 3, 8, 8, 2);
    runTest(true);
}

TEST_F(ktxTextureWriteTestRGB8, WriteCubemap) {
    helper.resize(createFlagBits::eNone, 1, 6, 2, 32, 32, 1);
    runTest(true);
}

TEST_F(ktxTextureWriteTestRGBA8, WriteCubemapMipmap) {
    helper.resize(createFlagBits::eMipmapped,
                  1, 6, 2, 32, 32, 1);
    runTest(true);
}
TEST_F(ktxTextureWriteTestRGBA8, WriteCubemapArrayMipmap) {
    helper.resize(createFlagBits::eMipmapped | createFlagBits::eArray,
                  4, 6, 2, 32, 32, 1);
    runTest(true);
}

TEST_F(ktxTextureWriteTestRG16, Write2DMipmap) {
    helper.resize(createFlagBits::eMipmapped, 1, 1, 2, 32, 32, 1);
    runTest(true);
}

}  // namespace
