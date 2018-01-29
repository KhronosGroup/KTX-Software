/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/**
 * @internal
 * @file unittests.cc
 * @~English
 *
 * @brief Tests of internal API functions.
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
extern "C" {
  #include "ktxint.h"
  #include "ktxfilestream.h"
  #include "ktxmemstream.h"
}
#include "gtest/gtest.h"

namespace {
    
//////////////////////////////
// MemStreamTest
//////////////////////////////

class CheckHeaderTest : public ::testing::Test {
  protected:
    CheckHeaderTest() {
        // Done this way rather than by using an initializer here
        // so it will compile on VS 2008.
        memcpy(testHeader.identifier, ktxId, sizeof(ktxId));
 
        testHeader.endianness = 0x04030201;  
        testHeader.glType = GL_UNSIGNED_BYTE;
        testHeader.glTypeSize = 1;
        testHeader.glFormat = GL_RGBA;
        testHeader.glInternalFormat = GL_RGBA8;
        testHeader.glBaseInternalFormat = GL_RGBA8;
        testHeader.pixelWidth = 16;
        testHeader.pixelHeight = 16;
        testHeader.pixelDepth = 16;
        testHeader.numberOfArrayElements = 0;
        testHeader.numberOfFaces = 1;
        testHeader.numberOfMipmapLevels = 5;
        testHeader.bytesOfKeyValueData = 0;
    }
    
    KTX_header testHeader;
    static ktx_uint8_t ktxId[12];
};

ktx_uint8_t CheckHeaderTest::ktxId[12] = {
    0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
};
    
TEST_F(CheckHeaderTest, AssertsOnNullArguments) {
    ASSERT_DEATH_IF_SUPPORTED(_ktxCheckHeader(0, 0), "Assert*");
}

TEST_F(CheckHeaderTest, ValidatesIdentifier) {
    KTX_supplemental_info suppInfo;

    EXPECT_EQ(_ktxCheckHeader(&testHeader, &suppInfo), KTX_SUCCESS);

    testHeader.identifier[9] = 0;
    EXPECT_EQ(_ktxCheckHeader(&testHeader, &suppInfo), KTX_UNKNOWN_FILE_FORMAT);
}
    
TEST_F(CheckHeaderTest, DisallowsInvalidEndianness) {
    KTX_supplemental_info suppInfo;

    testHeader.endianness = 0;
    EXPECT_EQ(_ktxCheckHeader(&testHeader, &suppInfo), KTX_FILE_DATA_ERROR);
}
    
//////////////////////////////
// MemStreamTest
//////////////////////////////

TEST(MemStreamTest, Read) {
    ktxStream stream;
    const ktx_uint8_t* data = (ktx_uint8_t*)"28 bytes of rubbish to read.";
    const size_t size = 28;
    char readBuf[size];
    
    ktxMemStream_construct_ro(&stream, data, size);
    stream.read(&stream, readBuf, size);
    EXPECT_EQ(memcmp(data, readBuf, size), 0);

	ktxMemStream_destruct(&stream);
}

TEST(MemStreamTest, Write) {
    ktxStream stream;
    const ktx_uint8_t* data = (ktx_uint8_t*)"29 bytes of rubbish to write.";
    const size_t count = 29;
	size_t returnedCount;
	ktx_uint8_t* returnedData;
    
    ktxMemStream_construct(&stream);
    stream.write(&stream, data, 1, count);
    
	stream.getsize(&stream, &returnedCount);
	ktxMemStream_getdata(&stream, &returnedData);
    EXPECT_EQ(returnedCount, count);
    EXPECT_EQ(memcmp(data, returnedData, count), 0);
    
	ktxMemStream_destruct(&stream);
}
    
TEST(MemStreamTest, WriteExpand) {
    ktxStream stream;
    const ktx_uint8_t* data = (ktx_uint8_t*)"29 bytes of rubbish to write.";
    const ktx_uint8_t* data2 = (ktx_uint8_t*)" 26 more bytes of rubbish.";
    const size_t count = 29;
    const size_t count2 = 26;
	size_t returnedCount;
	ktx_uint8_t* returnedData;
    
    ktxMemStream_construct(&stream);
    stream.write(&stream, data, 1, count);
    stream.write(&stream, data2, 1, count2);
	stream.getsize(&stream, &returnedCount);
	EXPECT_EQ(returnedCount, count + count2);
	ktxMemStream_getdata(&stream, &returnedData);
	EXPECT_EQ(memcmp(data, returnedData, count), 0);
	EXPECT_EQ(memcmp(data2, &returnedData[count], count2), 0);
    
	ktxMemStream_destruct(&stream);
}

}  // namespace
