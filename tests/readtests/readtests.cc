/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/**
 * @internal
 * @file unittests.cc
 * @~English
 *
 * @brief Test ktxReadKTX API functions.
 *
 * @author Mark Callow, Edgewise Consulting
 */

/*
 Copyright (c) 2010 The Khronos Group Inc.
 
 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and/or associated documentation files (the
 "Materials"), to deal in the Materials without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Materials, and to
 permit persons to whom the Materials are furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice shall be included
 unaltered in all copies or substantial portions of the Materials.
 Any additions, deletions, or changes to the original source files
 must be clearly indicated in accompanying documentation.
 
 If only executable code is distributed, then the accompanying
 documentation must state that "this software is based in part on the
 work of the Khronos Group."
 
 THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#if defined(_WIN32) && _MSC_VER < 1900
  #define snprintf _snprintf
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include <string.h>
#include "GL/glcorearb.h"
#include "ktx.h"
#include "gtest/gtest.h"

namespace {

///////////////////////////////////////////////////////////
// Test fixtures
///////////////////////////////////////////////////////////

//----------------------------------------------------
// Base fixture for ktxReadKTX and related test cases.
//----------------------------------------------------

class ReadKTXTestBase : public ::testing::Test {
  protected:
    ReadKTXTestBase() : pixelSize(16) {
        
        // Create a KTX file in memory for testing.
        
        KTX_error_code errorCode;
        
        ktxMemFile = 0;
        imageCbCalls = 0;

        mipLevels = levelsFromSize(pixelSize);

        texInfo.glType = GL_UNSIGNED_BYTE;
        texInfo.glTypeSize = 1;
        texInfo.glFormat = GL_RGBA;
        texInfo.glInternalFormat = GL_RGBA8;
        texInfo.glBaseInternalFormat = GL_RGBA8;
        texInfo.pixelWidth = pixelSize;
        texInfo.pixelHeight = pixelSize;
        texInfo.pixelDepth = 0;
        texInfo.numberOfArrayElements = 0;
        texInfo.numberOfFaces = 1;
        texInfo.numberOfMipmapLevels = mipLevels;
        
        // Create key-value data

        kvData = NULL;
        kvDataLen = 0;
        KTX_hash_table ht = ktxHashTable_Create();
        char orientation[10];
        snprintf(orientation, sizeof(orientation), KTX_ORIENTATION2_FMT,
                 'r', 'd');
        ktxHashTable_AddKVPair(ht, KTX_ORIENTATION_KEY,
                               (unsigned int)strlen(orientation) + 1,
                               orientation);
        errorCode = ktxHashTable_Serialize(ht, &kvDataLen, &kvData);
        if (KTX_SUCCESS != errorCode) {
            ADD_FAILURE() << "ktxHashTable_Serialize failed: "
                          << ktxErrorString(errorCode);;
            return;
        }
        ktxHashTable_Destroy(ht);

        // Initialize color table

        colors.push_back(rgba8color(0xff, 0x00, 0x00, 0xff));
        colors.push_back(rgba8color(0x00, 0xff, 0x00, 0xff));
        colors.push_back(rgba8color(0x00, 0x00, 0xff, 0xff));
        colors.push_back(rgba8color(0xff, 0xff, 0x00, 0xff));
        colors.push_back(rgba8color(0x00, 0xff, 0xff, 0xff));

        // Create images

        imageData.resize(5);
        images.resize(5);
        for (int i = 0; i < mipLevels; i++) {
            int levelSize = pixelSize >> i;
            // 2D texture -> h * w
            int pixelCount = levelSize * levelSize;
            // Allocate the image data and initialize it to a color.
            imageData[i].assign(pixelCount, colors[i]);
            // Type RGBA UNSIGNED_BYTE -> *4
            images[i].size = pixelCount * 4;
            images[i].data = (GLubyte*)&imageData[i].front().color;
        }
        
        // Create the in-memory KTX file

        errorCode = ktxWriteKTXM(&ktxMemFile, &ktxMemFileLen,
                                 &texInfo, kvDataLen, kvData,
                                 mipLevels, &images.front());
       if (KTX_SUCCESS != errorCode) {
            ADD_FAILURE() << "ktxWriteKTXM failed: "
                          << ktxErrorString(errorCode);
       }
    }
    
    ~ReadKTXTestBase() {
        if (ktxMemFile != NULL) delete ktxMemFile;
        if (kvData != NULL) delete kvData;
    }
    
    KTX_error_code KTXAPIENTRY
    imageCallback(int miplevel, int face,
                  int width, int height, int depth,
                  int layers,
                  ktx_uint32_t faceLodSize,
                  void* pixels)
    {
        int expectedWidth = pixelSize >> miplevel;
        EXPECT_EQ(width, expectedWidth);
        EXPECT_EQ(faceLodSize, expectedWidth * expectedWidth * 4);
        EXPECT_EQ(memcmp(pixels, images[miplevel].data, images[miplevel].size),
                  0);
        imageCbCalls++;
        return KTX_SUCCESS;
    }

    static KTX_error_code KTXAPIENTRY
    imageCallback(int miplevel, int face,
                  int width, int height,
                  int depth, int layers,
                  ktx_uint32_t faceLodSize,
                  void* pixels, void* userdata)
    {
        ReadKTXTestBase* fixture = (ReadKTXTestBase*)userdata;
        return fixture->imageCallback(miplevel, face, width, height, depth,
                                      layers, faceLodSize, pixels);
    }

    static int
    levelsFromSize(int pixelSize) {
        int mipLevels;
        for (mipLevels = 0; pixelSize != 1; mipLevels++, pixelSize >>= 1) { }
        return mipLevels;
    }

    unsigned char* ktxMemFile;
    GLsizei ktxMemFileLen;
    const int pixelSize;
    int mipLevels;
    unsigned char* kvData;
    unsigned int kvDataLen;
    unsigned int imageCbCalls;
    KTX_texture_info texInfo;
    union rgba8color {
        // Sadly anonymous union members are not so portable.
        struct { ktx_uint8_t r, g, b, a; } components;
        ktx_uint32_t color;

        rgba8color(ktx_uint8_t r, ktx_uint8_t g, ktx_uint8_t b, ktx_uint8_t a) {
            this->components.r = r;
            this->components.g = g;
            this->components.b = b;
            this->components.a = a;
        };
        rgba8color(const rgba8color& color) {
            this->color = color.color;
        };
    };
    std::vector< std::vector<rgba8color> > imageData;
    std::vector<KTX_image_info> images;
    std::vector<rgba8color> colors;
};


//---------------------------
// Actual test fixtures
//---------------------------

class ktxReader_readHeaderTest : public ReadKTXTestBase { };
class ktxReader_readKVDataTest : public ReadKTXTestBase { };
class ktxReader_readImagesTest : public ReadKTXTestBase { };
    

/////////////////////////////////////////
// ktxReader_readHeader tests
////////////////////////////////////////

TEST_F(ktxReader_readHeaderTest, InvalidValueOnNullParam) {
    KTX_reader reader;
    KTX_header header;
    KTX_supplemental_info suppInfo;

    EXPECT_EQ(ktxReader_readHeader(0, &header, &suppInfo), KTX_INVALID_VALUE);
    EXPECT_EQ(ktxReader_readHeader(&reader, 0, &suppInfo), KTX_INVALID_VALUE);
    EXPECT_EQ(ktxReader_readHeader(&reader, &header, 0), KTX_INVALID_VALUE);
}
    
TEST_F(ktxReader_readHeaderTest, InvalidOperationWhenCtxStateNotStart) {
    KTX_reader reader;
    KTX_header header;
    KTX_supplemental_info suppInfo;

    if (ktxMemFile != NULL) {
        KTX_error_code errorCode;

        errorCode = ktxOpenKTXM(ktxMemFile, ktxMemFileLen, &reader);
        ASSERT_TRUE(reader != NULL) << "ktxOpenKTXM failed: "
                                 << ktxErrorString(errorCode);
        EXPECT_EQ(ktxReader_readHeader(reader, &header, &suppInfo), KTX_SUCCESS);
        EXPECT_EQ(ktxReader_readHeader(reader, &header, &suppInfo), KTX_INVALID_OPERATION);
        EXPECT_EQ(ktxReader_close(reader), KTX_SUCCESS);
    }
}

TEST_F(ktxReader_readHeaderTest, ReadHeader) {
    KTX_reader reader;
    KTX_header header;
    KTX_supplemental_info suppInfo;
    KTX_error_code errorCode;

    if (ktxMemFile != NULL) {
        errorCode = ktxOpenKTXM(ktxMemFile, ktxMemFileLen, &reader);
        ASSERT_TRUE(reader != NULL) << "ktxOpenKTXM failed: "
                                 << ktxErrorString(errorCode);
        EXPECT_EQ(ktxReader_readHeader(reader, &header, &suppInfo), KTX_SUCCESS);
        
        EXPECT_EQ(memcmp(&header.glType, &texInfo, sizeof(texInfo)), 0);
        EXPECT_EQ(suppInfo.compressed, 0);
        EXPECT_EQ(suppInfo.generateMipmaps, 0);
        EXPECT_EQ(suppInfo.textureDimension, 2);
        EXPECT_EQ(ktxReader_close(reader), KTX_SUCCESS);
    }
}

/////////////////////////////////////////
// ktxReader_readKVData tests
////////////////////////////////////////
    
TEST_F(ktxReader_readKVDataTest, InvalidValueOnNullContext) {
    ktx_uint32_t kvdLen;
    ktx_uint8_t* kvd;
    
    EXPECT_EQ(ktxReader_readKVData(0, &kvdLen, &kvd), KTX_INVALID_VALUE);
}
    
TEST_F(ktxReader_readKVDataTest, ReadKVData) {
    KTX_reader reader;
    KTX_header header;
    KTX_supplemental_info suppInfo;
    KTX_error_code errorCode;
    ktx_uint32_t kvdLen;
    ktx_uint8_t* kvd;

    if (ktxMemFile != NULL) {
        errorCode = ktxOpenKTXM(ktxMemFile, ktxMemFileLen, &reader);
        ASSERT_TRUE(reader != NULL) << "ktxOpenKTXM failed: "
                                 << ktxErrorString(errorCode);
        ASSERT_EQ(ktxReader_readHeader(reader, &header, &suppInfo), KTX_SUCCESS);
        
        EXPECT_EQ(ktxReader_readKVData(reader, &kvdLen, &kvd), KTX_SUCCESS);
        EXPECT_EQ(kvdLen, kvDataLen) << "Length of KV data incorrect";
        EXPECT_EQ(memcmp(kvd, kvData, kvDataLen), 0);
        EXPECT_EQ(ktxReader_close(reader), KTX_SUCCESS);
    }
}
    
/////////////////////////////////////////
// ktxReader_readImagestests
////////////////////////////////////////
    
TEST_F(ktxReader_readImagesTest, InvalidValueOnNullCallback) {
    KTX_reader reader;
    KTX_header header;
    KTX_supplemental_info suppInfo;
    KTX_error_code errorCode;
    ktxReader_readImagesTest* fixture = this;
    
    if (ktxMemFile != NULL) {
        errorCode = ktxOpenKTXM(ktxMemFile, ktxMemFileLen, &reader);
        ASSERT_TRUE(reader != NULL) << "ktxOpenKTXM failed: "
        << ktxErrorString(errorCode);
        ASSERT_EQ(ktxReader_readHeader(reader, &header, &suppInfo), KTX_SUCCESS);
        ASSERT_EQ(ktxReader_readKVData(reader, 0, 0), KTX_SUCCESS);
        
        EXPECT_EQ(ktxReader_readImages(reader, 0, fixture), KTX_INVALID_VALUE);
        EXPECT_EQ(ktxReader_close(reader), KTX_SUCCESS);
    }
}

TEST_F(ktxReader_readImagesTest, ReadImages) {
    KTX_reader reader;
    KTX_header header;
    KTX_supplemental_info suppInfo;
    KTX_error_code errorCode;
    ktx_uint32_t kvdLen;
    ktx_uint8_t* kvd;
    ktxReader_readImagesTest* fixture = this;

    
    if (ktxMemFile != NULL) {
        errorCode = ktxOpenKTXM(ktxMemFile, ktxMemFileLen, &reader);
        ASSERT_TRUE(reader != NULL) << "ktxOpenKTXM failed: "
                                 << ktxErrorString(errorCode);
        ASSERT_EQ(ktxReader_readHeader(reader, &header, &suppInfo), KTX_SUCCESS);
        ASSERT_EQ(ktxReader_readKVData(reader, &kvdLen, &kvd), KTX_SUCCESS);
        
        EXPECT_EQ(ktxReader_readImages(reader, imageCallback, fixture), KTX_SUCCESS);
        EXPECT_EQ(imageCbCalls, mipLevels)
                  << "No. of calls to imageCallback differs from number of mip levels";
        EXPECT_EQ(ktxReader_close(reader), KTX_SUCCESS);
    }
}
    
}  // namespace
