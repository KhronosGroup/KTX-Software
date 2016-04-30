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

#include <string.h>
#include "GL/glcorearb.h"
#include "ktx.h"
#include "gtest/gtest.h"

#if defined(_WIN32) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

namespace {
    

//////////////////////////////
// ReadKTXTest
//////////////////////////////

class ReadKTXTest : public ::testing::Test {
protected:
    ReadKTXTest() {
        
        // Create a KTX file in memory for testing.
        
        KTX_error_code errorCode;
        
        ktxMemFile = 0;
        images = 0;
        
        pixelSize = 16;
        // log2
        int temp = pixelSize;
        for (mipLevels = 0; temp != 1; mipLevels++, temp >>= 1) { }
        mipLevels++;
        
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
        
        images = new KTX_image_info[mipLevels];
        for (int i = 0; i < mipLevels; i++) {
            int faceLodSize;
            int levelSize = pixelSize >> i;
            // 2D texture and type RGBA UNSIGNED_BYTE (*4)
            images[i].size = levelSize * levelSize * 4;
            images[i].data = new GLubyte[images[i].size];
            int pattern = 0xFF0000FF;
            memset_pattern4(images[i].data, &pattern, images[i].size);
        }
        
        errorCode = ktxWriteKTXM(&ktxMemFile, &ktxMemFileLen,
                                 &texInfo, kvDataLen, kvData,
                                 mipLevels, images);
       if (KTX_SUCCESS != errorCode) {
            ADD_FAILURE() << "ktxWriteKTXM failed: "
                          << ktxErrorString(errorCode);
       }
    }
    
    ~ReadKTXTest() {
        if (ktxMemFile != NULL) delete ktxMemFile;
        if (kvData != NULL) delete kvData;
        if (images != NULL) {
            for (int i = 0; i < mipLevels; i++)
                delete images[i].data;
        }
    }
    
    KTX_error_code KTXAPIENTRY
    imageCallback(int miplevel, int face,
                  int width, int heightOrLayers,
                  int depthOrLayers,
                  ktx_uint32_t faceLodSize,
                  void* pixels)
    {
        int expectedWidth = pixelSize >> miplevel;
        EXPECT_EQ(width, expectedWidth);
        EXPECT_EQ(faceLodSize, expectedWidth * expectedWidth * 4);
        EXPECT_EQ(memcmp(pixels, images[miplevel].data, images[miplevel].size),
                  0);
        return KTX_SUCCESS;
    }

    static KTX_error_code KTXAPIENTRY
    imageCallback(int miplevel, int face,
                  int width, int heightOrLayers,
                  int depthOrLayers,
                  ktx_uint32_t faceLodSize,
                  void* pixels, void* userdata)
    {
        ReadKTXTest* fixture = (ReadKTXTest*)userdata;
        return fixture->imageCallback(miplevel, face, width, heightOrLayers,
                                      depthOrLayers, faceLodSize, pixels);
    }
    
    
    unsigned char* ktxMemFile;
    GLsizei ktxMemFileLen;
    int pixelSize;
    int mipLevels;
    unsigned char* kvData;
    unsigned int kvDataLen;
    KTX_texture_info texInfo;
    KTX_image_info* images;
};
    
TEST(ktxReadHeaderTest, InvalidValueOnNullParam) {
    KTX_context ctx;
    KTX_header header;
    KTX_supplemental_info suppInfo;

    EXPECT_EQ(ktxReadHeader(0, &header, &suppInfo), KTX_INVALID_VALUE);
    EXPECT_EQ(ktxReadHeader(&ctx, 0, &suppInfo), KTX_INVALID_VALUE);
    EXPECT_EQ(ktxReadHeader(&ctx, &header, 0), KTX_INVALID_VALUE);
}

TEST(ktxReadKVDataTest, InvalidValueOnNullContext) {
    KTX_context ctx;
    ktx_uint32_t kvdLen;
    ktx_uint8_t* kvd;

    EXPECT_EQ(ktxReadKVData(0, &kvdLen, &kvd), KTX_INVALID_VALUE);
}

TEST_F(ReadKTXTest, TestReadHeader) {
    KTX_context ctx;
    KTX_header header;
    KTX_supplemental_info suppInfo;
    KTX_error_code errorCode;

    if (ktxMemFile != NULL) {
        errorCode = ktxOpenKTXM(ktxMemFile, ktxMemFileLen, &ctx);
        ASSERT_TRUE(ctx != NULL) << "ktxOpenKTXM failed: "
                                 << ktxErrorString(errorCode);
        EXPECT_EQ(ktxReadHeader(ctx, &header, &suppInfo), KTX_SUCCESS);
        
        EXPECT_EQ(memcmp(&header.glType, &texInfo, sizeof(texInfo)), 0);
        EXPECT_EQ(suppInfo.compressed, 0);
        EXPECT_EQ(suppInfo.generateMipmaps, 0);
        EXPECT_EQ(suppInfo.textureDimension, 2);
    }
}

TEST_F(ReadKTXTest, TestReadKVData) {
    KTX_context ctx;
    KTX_header header;
    KTX_supplemental_info suppInfo;
    KTX_error_code errorCode;
    ktx_uint32_t kvdLen;
    ktx_uint8_t* kvd;

    if (ktxMemFile != NULL) {
        errorCode = ktxOpenKTXM(ktxMemFile, ktxMemFileLen, &ctx);
        ASSERT_TRUE(ctx != NULL) << "ktxOpenKTXM failed: "
                                 << ktxErrorString(errorCode);
        ASSERT_EQ(ktxReadHeader(ctx, &header, &suppInfo), KTX_SUCCESS);
        
        EXPECT_EQ(ktxReadKVData(ctx, &kvdLen, &kvd), KTX_SUCCESS);
        EXPECT_EQ(kvdLen, kvDataLen) << "Length of KV data incorrect";
        EXPECT_EQ(memcmp(kvd, kvData, kvDataLen), 0);
    }
}
    
TEST_F(ReadKTXTest, TestReadImages) {
    KTX_context ctx;
    KTX_header header;
    KTX_supplemental_info suppInfo;
    KTX_error_code errorCode;
    ktx_uint32_t kvdLen;
    ktx_uint8_t* kvd;
    
    if (ktxMemFile != NULL) {
        errorCode = ktxOpenKTXM(ktxMemFile, ktxMemFileLen, &ctx);
        ASSERT_TRUE(ctx != NULL) << "ktxOpenKTXM failed: "
                                 << ktxErrorString(errorCode);
        ASSERT_EQ(ktxReadHeader(ctx, &header, &suppInfo), KTX_SUCCESS);
        ASSERT_EQ(ktxReadKVData(ctx, &kvdLen, &kvd), KTX_SUCCESS);
        
        EXPECT_EQ(ktxReadImages(ctx, imageCallback, this), KTX_SUCCESS);
    }
}
    
}  // namespace
