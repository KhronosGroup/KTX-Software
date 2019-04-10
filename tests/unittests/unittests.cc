/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/*
 * ©2010-2018 Mark Callow.
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

/**
 * @internal
 * @file unittests.cc
 * @~English
 *
 * @brief Tests of internal API functions.
 *
 * @author Mark Callow, Edgewise Consulting
 */

#if defined(_WIN32)
  #define _CRT_SECURE_NO_WARNINGS
  #if _MSC_VER < 1900
    #define snprintf _snprintf
  #endif
#endif

#include <string.h>
#include "GL/glcorearb.h"
#include "gl_format.h"
#include "ktx.h"
extern "C" {
  #include "ktxint.h"
  #include "filestream.h"
  #include "memstream.h"
}
#include "gtest/gtest.h"
#include "wthelper.h"

namespace {
    
///////////////////////////////////////////////////////////
// Test fixtures
///////////////////////////////////////////////////////////

//--------------------------------------------
// Fixture for CheckHeaderTest
//--------------------------------------------

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
        testHeader.glInternalformat = GL_RGBA8;
        testHeader.glBaseInternalformat = GL_RGBA8;
        testHeader.pixelWidth = 16;
        testHeader.pixelHeight = 16;
        testHeader.pixelDepth = 16;
        testHeader.numberOfArrayElements = 0;
        testHeader.numberOfFaces = 1;
        testHeader.numberOfMipmapLevels = 5;
        testHeader.bytesOfKeyValueData = 0;
    }
    
    KTX_header testHeader;
  public:
    static ktx_uint8_t ktxId[12];
};

ktx_uint8_t CheckHeaderTest::ktxId[12] = {
    0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
};
    
//--------------------------------------------
// Base fixture for WriterTestHelper tests.
//--------------------------------------------

template<typename component_type, ktx_uint32_t num_components,
         GLenum internalformat>
class WriterTestHelperTestBase : public ::testing::Test {
  public:
    WriterTestHelperTestBase() { }

    WriterTestHelper<component_type, num_components, internalformat> helper;

   ktx_uint32_t numComponents() { return num_components; }
};

class WriterTestHelperRGBA8Test : public WriterTestHelperTestBase<GLubyte, 4, GL_RGBA8> { };
class WriterTestHelperRGB8Test : public WriterTestHelperTestBase<GLubyte, 3, GL_RGB8> { };
typedef WriterTestHelper<GLubyte, 4, GL_RGBA8>::createFlagBits createFlagBits;

//--------------------------------------------
// Base fixture for ktxWriteKTX tests.
//--------------------------------------------

template<typename component_type, ktx_uint32_t numComponents,
         GLenum internalformat>
class ktxWriteKTXTestBase : public ::testing::Test {
  public:
    ktxWriteKTXTestBase() { }
    
    void runTest(bool writeMetadata) {
        unsigned char* ktxMemFile;
        GLsizei ktxMemFileLen;
        ktx_uint8_t* filePtr;
        KTX_error_code result;

        result = ktxWriteKTXM(&ktxMemFile, &ktxMemFileLen,
                              &helper.texinfo,
                              writeMetadata ? helper.kvDataLen : 0,
                              writeMetadata ? helper.kvData : nullptr,
                              (GLuint)helper.imageList.size(),
                              helper.imageList.data());
        ASSERT_TRUE(result == KTX_SUCCESS) << "ktxWriteKTXM failed: "
                                           << ktxErrorString(result);
        EXPECT_EQ(memcmp(ktxMemFile, CheckHeaderTest::ktxId,
                         sizeof(CheckHeaderTest::ktxId)),
                  0);
        EXPECT_EQ(helper.texinfo.compare((KTX_header*)ktxMemFile), true);
        filePtr = ktxMemFile + sizeof(KTX_header);
        if (writeMetadata) {
            EXPECT_EQ(memcmp(filePtr, helper.kvData, helper.kvDataLen), 0);
            filePtr += helper.kvDataLen;
        }
        EXPECT_EQ(helper.compareRawImages(ktxMemFile + sizeof(KTX_header)), true);
        delete ktxMemFile;
    }

    WriterTestHelper<component_type, numComponents, internalformat> helper;
};

class ktxWriteKTXRGBA8Test : public ktxWriteKTXTestBase<GLubyte, 4, GL_RGBA8> { };
class ktxWriteKTXRGB8Test : public ktxWriteKTXTestBase<GLubyte, 3, GL_RGB8> { };

//////////////////////////////
// CheckHeaderTest
//////////////////////////////

#if defined(DEBUG)
TEST_F(CheckHeaderTest, AssertsOnNullArguments) {
    ASSERT_DEATH_IF_SUPPORTED(_ktxCheckHeader(0, 0), "Assert*");
}
#endif

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
    
    ktxMemStream_construct(&stream, KTX_TRUE);
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
    
    ktxMemStream_construct(&stream, KTX_TRUE);
    stream.write(&stream, data, 1, count);
    stream.write(&stream, data2, 1, count2);
	stream.getsize(&stream, &returnedCount);
	EXPECT_EQ(returnedCount, count + count2);
	ktxMemStream_getdata(&stream, &returnedData);
	EXPECT_EQ(memcmp(data, returnedData, count), 0);
	EXPECT_EQ(memcmp(data2, &returnedData[count], count2), 0);
    
	ktxMemStream_destruct(&stream);
}

//////////////////////////////
// WriterTestHelper tests.
//////////////////////////////

TEST_F(WriterTestHelperRGB8Test, Construct2D) {
    helper.resize(createFlagBits::eNone, 1, 1, 2, 32, 32, 1);
    EXPECT_EQ(helper.images.size(), 1);
    EXPECT_EQ(helper.images[0].size(), 1);
    EXPECT_EQ(helper.images[0][0].size(), 1);
    EXPECT_EQ(helper.images[0][0][0].size(), 32 * 32 * 3);
    EXPECT_EQ(numComponents(), 3);
}

TEST_F(WriterTestHelperRGB8Test, Construct3D) {
    helper.resize(createFlagBits::eNone, 1, 1, 3, 32, 32, 32);
    EXPECT_EQ(helper.images.size(), 1);
    EXPECT_EQ(helper.images[0].size(), 1);
    EXPECT_EQ(helper.images[0][0].size(), 32);
    EXPECT_EQ(helper.images[0][0][0].size(), 32 * 32 * 3);
    EXPECT_EQ(numComponents(), 3);
}

//////////////////////////////
// ktxWriteKTX tests.
//////////////////////////////

TEST_F(ktxWriteKTXRGBA8Test, InvalidOpOnMismatchedFormats) {
    helper.resize(createFlagBits::eNone, 1, 1, 2, 32, 32, 1);
    unsigned char* ktxMemFile;
    GLsizei ktxMemFileLen;
    KTX_error_code result;

    helper.texinfo.glFormat = GL_RGB;
    helper.texinfo.glBaseInternalFormat = GL_RGB;

    result = ktxWriteKTXM(&ktxMemFile, &ktxMemFileLen,
                          &helper.texinfo, 0, nullptr,
                          (GLuint)helper.imageList.size(),
                          helper.imageList.data());

    EXPECT_EQ(result, KTX_INVALID_OPERATION);
}

TEST_F(ktxWriteKTXRGB8Test, Write1D) {
    helper.resize(createFlagBits::eNone, 1, 1, 1, 32, 1, 1);
    runTest(false);
}

TEST_F(ktxWriteKTXRGB8Test, Write1DNeedsPadding) {
    helper.resize(createFlagBits::eNone, 1, 1, 1, 9, 1, 1);
    runTest(false);
}

TEST_F(ktxWriteKTXRGBA8Test, Write1DMipmap) {
    helper.resize(createFlagBits::eMipmapped, 1, 1, 1, 32, 1, 1);
    runTest(false);
}
TEST_F(ktxWriteKTXRGB8Test, Write1DArray) {
    helper.resize(createFlagBits::eArray, 4, 1, 1, 32, 1, 1);
    runTest(false);
}

TEST_F(ktxWriteKTXRGBA8Test, Write1DArrayMipmap) {
    helper.resize(createFlagBits::eMipmapped | createFlagBits::eArray,
                  4, 1, 1, 32, 1, 1);
    runTest(false);
}

TEST_F(ktxWriteKTXRGB8Test, Write2D) {
    helper.resize(createFlagBits::eNone, 1, 1, 2, 32, 32, 1);
    runTest(false);
}

TEST_F(ktxWriteKTXRGB8Test, Write2DNeedsPadding) {
    helper.resize(createFlagBits::eNone, 1, 1, 2, 9, 18, 1);
    runTest(false);
}

TEST_F(ktxWriteKTXRGB8Test, Write2DArray) {
    helper.resize(createFlagBits::eArray, 4, 1, 2, 32, 32, 1);
    runTest(false);
}

TEST_F(ktxWriteKTXRGBA8Test, Write2D) {
    helper.resize(createFlagBits::eNone, 1, 1, 2, 32, 32, 1);
    runTest(false);
}

TEST_F(ktxWriteKTXRGBA8Test, Write2DMipmap) {
    helper.resize(createFlagBits::eMipmapped, 1, 1, 2, 32, 32, 1);
    runTest(false);
}

TEST_F(ktxWriteKTXRGBA8Test, Write2DArray) {
    helper.resize(createFlagBits::eArray, 4, 1, 2, 32, 32, 1);
    runTest(false);
}

TEST_F(ktxWriteKTXRGBA8Test, Write2DArrayMipmap) {
    helper.resize(createFlagBits::eMipmapped | createFlagBits::eArray,
                  4, 1, 2, 32, 32, 1);
    runTest(false);
}

TEST_F(ktxWriteKTXRGBA8Test, Write3D) {
    helper.resize(createFlagBits::eNone, 1, 1, 3, 32, 32, 32);
    runTest(false);
}

TEST_F(ktxWriteKTXRGBA8Test, Write3DMipmap) {
    helper.resize(createFlagBits::eMipmapped, 1, 1, 3, 32, 32, 32);
    runTest(false);
}

// KTX can support 3D array textures so we let them be created, even though
// neither OpenGL nor Vulkan support them. Note that _checkheader throws
// KTX_UNSUPPORTED_TEXTURE_TYPE when trying to load one of these.
TEST_F(ktxWriteKTXRGBA8Test, Write3DArray) {
    helper.resize(createFlagBits::eArray, 4, 1, 3, 32, 32, 32);
    runTest(false);
}

TEST_F(ktxWriteKTXRGBA8Test, Write3DArrayMipmap) {
    helper.resize(createFlagBits::eMipmapped | createFlagBits::eArray,
                  4, 1, 3, 32, 32, 32);
    runTest(false);
}

TEST_F(ktxWriteKTXRGBA8Test, WriteCubemap) {
    helper.resize(createFlagBits::eNone, 1, 6, 2, 32, 32, 1);
    runTest(false);
}

TEST_F(ktxWriteKTXRGBA8Test, WriteCubemapMipmap) {
    helper.resize(createFlagBits::eMipmapped, 1, 6, 2, 32, 32, 1);
    runTest(false);
}

TEST_F(ktxWriteKTXRGBA8Test, WriteCubemapArray) {
    helper.resize(createFlagBits::eArray, 4, 6, 2, 32, 32, 1);
    runTest(false);
}

TEST_F(ktxWriteKTXRGBA8Test, WriteCubemapArrayMipmap) {
    helper.resize(createFlagBits::eMipmapped | createFlagBits::eArray,
                  4, 6, 2, 32, 32, 1);
    runTest(false);
}

}  // namespace
