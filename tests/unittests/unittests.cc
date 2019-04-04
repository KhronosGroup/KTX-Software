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

/////////////////////////////////////
// Base fixture for createDFD tests.
/////////////////////////////////////

#include <KHR/khr_df.h>
#include "dfdutils/dfd.h"

// Template for single plane formats.
template<ktx_uint32_t numComponents, ktx_uint32_t bytesPlane>
class createDFDTestBase : public ::testing::Test {
  public:
    struct sampleType {
        uint32_t bitOffset: 16;
        uint32_t bitLength: 8;
        uint32_t channelType: 8; // Includes qualifiers
        uint32_t samplePosition0: 8;
        uint32_t samplePosition1: 8;
        uint32_t samplePosition2: 8;
        uint32_t samplePosition3: 8;
        uint32_t lower;
        uint32_t upper;
    };

    createDFDTestBase() {
        expected.vendorId = KHR_DF_VENDORID_KHRONOS;
        expected.descriptorType = KHR_DF_KHR_DESCRIPTORTYPE_BASICFORMAT;
        expected.versionNumber = KHR_DF_VERSIONNUMBER_1_3;
        // sizeof is after template instantiation so BDFD already includes
        // numComponents samples.
        expected.descriptorBlockSize = sizeof(BDFD);
        expected.bytesPlane0 = bytesPlane;
        expected.bytesPlane1 = 0;
        expected.bytesPlane2 = 0;
        expected.bytesPlane3 = 0;
        expected.bytesPlane4 = 0;
        expected.bytesPlane5 = 0;
        expected.bytesPlane6 = 0;
        expected.bytesPlane7 = 0;

    }

  protected:
    struct BDFD {
        uint32_t vendorId: 17;
        uint32_t descriptorType: 15;
        uint32_t versionNumber: 16;
        uint32_t descriptorBlockSize: 16;
        uint32_t model: 8;
        uint32_t primaries: 8;
        uint32_t transfer: 8;
        uint32_t flags: 8;
        uint32_t texelBlockDimension0: 8;
        uint32_t texelBlockDimension1: 8;
        uint32_t texelBlockDimension2: 8;
        uint32_t texelBlockDimension3: 8;
        uint32_t bytesPlane0: 8;
        uint32_t bytesPlane1: 8;
        uint32_t bytesPlane2: 8;
        uint32_t bytesPlane3: 8;
        uint32_t bytesPlane4: 8;
        uint32_t bytesPlane5: 8;
        uint32_t bytesPlane6: 8;
        uint32_t bytesPlane7: 8;
        sampleType samples[numComponents];
    } expected;
};

template<ktx_uint32_t numComponents, ktx_uint32_t bytesPlane>
class createDFDTestBaseUncomp : public createDFDTestBase<numComponents, bytesPlane> {
  using typename createDFDTestBase<numComponents, bytesPlane>::sampleType;
  public:
    createDFDTestBaseUncomp() : createDFDTestBase<numComponents, bytesPlane>() {
       expected.texelBlockDimension0 = 0;
       expected.texelBlockDimension1 = 0;
       expected.texelBlockDimension2 = 0;
       expected.texelBlockDimension3 = 0;
    }

    void customize(uint8_t model, uint8_t primaries, uint8_t transfer,
                   uint8_t flags,
                   std::initializer_list<sampleType> samples) {
        expected.model = model;
        expected.primaries = primaries;
        expected.transfer = transfer;
        expected.flags = flags;

        uint32_t i = 0; // There's got to be some syntax for declaring this in the for
        for (auto sample : samples ) {
            expected.samples[i] = sample;
            if (++i == numComponents)
                break;
        }
    }

  protected:
    using createDFDTestBase<numComponents, bytesPlane>::expected;
};

template<ktx_uint32_t numComponents, ktx_uint32_t bytesPlane>
class createDFDTestBaseComp : public createDFDTestBase<numComponents, bytesPlane> {
  using typename createDFDTestBase<numComponents, bytesPlane>::sampleType;
  public:
    createDFDTestBaseComp() : createDFDTestBase<numComponents, bytesPlane>() {
       expected.texelBlockDimension2 = 0;
       expected.texelBlockDimension3 = 0;
    }

    void customize(uint8_t model, uint8_t primaries, uint8_t transfer,
                   uint8_t flags, ktx_uint32_t dim0, ktx_uint32_t dim1,
                   std::initializer_list<sampleType> samples) {
        expected.model = model;
        expected.primaries = primaries;
        expected.transfer = transfer;
        expected.flags = flags;
        expected.texelBlockDimension0 = dim0;
        expected.texelBlockDimension1 = dim1;


        uint32_t i = 0; // There's got to be some syntax for declaring this in the for
        for (auto sample : samples ) {
            expected.samples[i] = sample;
            if (++i == numComponents)
                break;
        }
    }

  protected:
    using createDFDTestBase<numComponents, bytesPlane>::expected;
};

class createDFDUnpackedTest4 : public createDFDTestBaseUncomp<4, 4> { };
class createDFDUnpackedTest3 : public createDFDTestBaseUncomp<3, 3> { };
class createDFDPackedTest3 : public createDFDTestBaseUncomp<3, 2> { };
class createDFDCompressedTest1 : public createDFDTestBaseComp<1, 8> { };
class createDFDCompressedTest2 : public createDFDTestBaseComp<2, 16> { };
class createDFDCompressedTest1x16 : public createDFDTestBaseComp<1, 16> { };

//////////////////////////////
// createDFD tests.
//////////////////////////////

TEST_F(createDFDUnpackedTest4, FormatSRGBA8) {
    customize(KHR_DF_MODEL_RGBSDA, KHR_DF_PRIMARIES_BT709,
              KHR_DF_TRANSFER_SRGB, KHR_DF_FLAG_ALPHA_STRAIGHT,
              {
                {0, 7, KHR_DF_CHANNEL_RGBSDA_RED, 0, 0, 0, 0, 0, 255},
                {8, 7, KHR_DF_CHANNEL_RGBSDA_GREEN, 0, 0, 0, 0, 0, 255},
                {16, 7, KHR_DF_CHANNEL_RGBSDA_BLUE, 0, 0, 0, 0, 0, 255},
                {24, 7, KHR_DF_CHANNEL_RGBSDA_ALPHA | KHR_DF_SAMPLE_DATATYPE_LINEAR,
                0, 0, 0, 0, 0, 255}
              }
             );

    uint32_t* dfd = createDFDUnpacked(KTX_FALSE, 4, 1, KTX_FALSE, s_SRGB);

    EXPECT_EQ(*dfd, sizeof(expected) + 4);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}

TEST_F(createDFDUnpackedTest4, FormatSBGRA8) {
    customize(KHR_DF_MODEL_RGBSDA, KHR_DF_PRIMARIES_BT709,
              KHR_DF_TRANSFER_SRGB, KHR_DF_FLAG_ALPHA_STRAIGHT,
              {
                {0, 7, KHR_DF_CHANNEL_RGBSDA_BLUE, 0, 0, 0, 0, 0, 255},
                {8, 7, KHR_DF_CHANNEL_RGBSDA_GREEN, 0, 0, 0, 0, 0, 255},
                {16, 7, KHR_DF_CHANNEL_RGBSDA_RED, 0, 0, 0, 0, 0, 255},
                {24, 7, KHR_DF_CHANNEL_RGBSDA_ALPHA | KHR_DF_SAMPLE_DATATYPE_LINEAR,
                0, 0, 0, 0, 0, 255}
              }
             );

    uint32_t* dfd = createDFDUnpacked(KTX_FALSE, 4, 1, KTX_TRUE, s_SRGB);

    EXPECT_EQ(*dfd, sizeof(expected) + 4);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}

TEST_F(createDFDUnpackedTest4, FormatRGBA8) {
    customize(KHR_DF_MODEL_RGBSDA, KHR_DF_PRIMARIES_BT709,
              KHR_DF_TRANSFER_LINEAR, KHR_DF_FLAG_ALPHA_STRAIGHT,
              {
                {0, 7, KHR_DF_CHANNEL_RGBSDA_RED, 0, 0, 0, 0, 0, 255},
                {8, 7, KHR_DF_CHANNEL_RGBSDA_GREEN, 0, 0, 0, 0, 0, 255},
                {16, 7, KHR_DF_CHANNEL_RGBSDA_BLUE, 0, 0, 0, 0, 0, 255},
                {24, 7, KHR_DF_CHANNEL_RGBSDA_ALPHA, 0, 0, 0, 0, 0, 255}
              }
             );

    uint32_t* dfd = createDFDUnpacked(KTX_FALSE, 4, 1, KTX_FALSE, s_UNORM);

    EXPECT_EQ(*dfd, sizeof(expected) + 4);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}

TEST_F(createDFDUnpackedTest3, FormatSRGB8) {
    customize(KHR_DF_MODEL_RGBSDA, KHR_DF_PRIMARIES_BT709,
              KHR_DF_TRANSFER_SRGB, KHR_DF_FLAG_ALPHA_STRAIGHT,
              {
                {0, 7, KHR_DF_CHANNEL_RGBSDA_RED, 0, 0, 0, 0, 0, 255},
                {8, 7, KHR_DF_CHANNEL_RGBSDA_GREEN, 0, 0, 0, 0, 0, 255},
                {16, 7, KHR_DF_CHANNEL_RGBSDA_BLUE, 0, 0, 0, 0, 0, 255},
              }
             );

    uint32_t* dfd = createDFDUnpacked(KTX_FALSE, 3, 1, KTX_FALSE, s_SRGB);

    EXPECT_EQ(*dfd, sizeof(expected) + 4);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}

TEST_F(createDFDPackedTest3, FormatRGB565) {
    customize(KHR_DF_MODEL_RGBSDA, KHR_DF_PRIMARIES_BT709,
              KHR_DF_TRANSFER_LINEAR, KHR_DF_FLAG_ALPHA_STRAIGHT,
              {
                {0, 4, KHR_DF_CHANNEL_RGBSDA_BLUE, 0, 0, 0, 0, 0, 31},
                {5, 5, KHR_DF_CHANNEL_RGBSDA_GREEN, 0, 0, 0, 0, 0, 63},
                {11, 4, KHR_DF_CHANNEL_RGBSDA_RED, 0, 0, 0, 0, 0, 31},
              }
             );

    // In order from LSB.
    int bits[] = {5, 6, 5, 0};
    int channels[] = {
        KHR_DF_CHANNEL_RGBSDA_BLUE,
        KHR_DF_CHANNEL_RGBSDA_GREEN,
        KHR_DF_CHANNEL_RGBSDA_RED,
        0
    };
    uint32_t* dfd = createDFDPacked(KTX_FALSE, 3, bits, channels, s_UNORM);

    EXPECT_EQ(*dfd, sizeof(expected) + 4);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}

TEST_F(createDFDCompressedTest1, FormatETC1S_R8B8G8) {
    customize(KHR_DF_MODEL_ETC1S, KHR_DF_PRIMARIES_BT709,
              KHR_DF_TRANSFER_LINEAR, KHR_DF_FLAG_ALPHA_STRAIGHT,
              3, 3,
              {
                {0, 63, KHR_DF_CHANNEL_ETC1S_COLOR, 0, 0, 0, 0, 0, 0xFFFFFFFF},
              }
             );

    uint32_t* dfd = createDFDCompressed(c_ETC1S, 4, 4, s_UNORM);

    EXPECT_EQ(*dfd, sizeof(expected) + 4);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}

TEST_F(createDFDCompressedTest1, FormatETC1S_SR8B8G8) {
    customize(KHR_DF_MODEL_ETC1S, KHR_DF_PRIMARIES_BT709,
              KHR_DF_TRANSFER_SRGB, KHR_DF_FLAG_ALPHA_STRAIGHT,
              3, 3,
              {
                {0, 63, KHR_DF_CHANNEL_ETC1S_COLOR, 0, 0, 0, 0, 0, 0xFFFFFFFF},
              }
             );

    uint32_t* dfd = createDFDCompressed(c_ETC1S, 4, 4, s_SRGB);

    EXPECT_EQ(*dfd, sizeof(expected) + 4);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}

TEST_F(createDFDCompressedTest1, FormatETC2_R8B8G8) {
    customize(KHR_DF_MODEL_ETC2, KHR_DF_PRIMARIES_BT709,
              KHR_DF_TRANSFER_LINEAR, KHR_DF_FLAG_ALPHA_STRAIGHT,
              3, 3,
              {
                {0, 63, KHR_DF_CHANNEL_ETC2_COLOR, 0, 0, 0, 0, 0, 0xFFFFFFFF},
              }
             );

    uint32_t* dfd = createDFDCompressed(c_ETC2_R8G8B8, 4, 4, s_UNORM);

    EXPECT_EQ(*dfd, sizeof(expected) + 4);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}

TEST_F(createDFDCompressedTest2, FormatETC2_R8G8B8A8) {
    customize(KHR_DF_MODEL_ETC2, KHR_DF_PRIMARIES_BT709,
              KHR_DF_TRANSFER_LINEAR, KHR_DF_FLAG_ALPHA_STRAIGHT,
              3, 3,
              {
                {0, 63, KHR_DF_CHANNEL_ETC2_ALPHA, 0, 0, 0, 0, 0, 0xFFFFFFFF},
                {64, 63, KHR_DF_CHANNEL_ETC2_COLOR, 0, 0, 0, 0, 0, 0xFFFFFFFF},
              }
             );

    uint32_t* dfd = createDFDCompressed(c_ETC2_R8G8B8A8, 4, 4, s_UNORM);

    EXPECT_EQ(*dfd, sizeof(expected) + 4);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}

TEST_F(createDFDCompressedTest1, FormatETC2_SR8B8G8) {
    customize(KHR_DF_MODEL_ETC2, KHR_DF_PRIMARIES_BT709,
              KHR_DF_TRANSFER_SRGB, KHR_DF_FLAG_ALPHA_STRAIGHT,
              3, 3,
              {
                {0, 63, KHR_DF_CHANNEL_ETC2_COLOR, 0, 0, 0, 0, 0, 0xFFFFFFFF},
              }
             );

    uint32_t* dfd = createDFDCompressed(c_ETC2_R8G8B8, 4, 4, s_SRGB);

    EXPECT_EQ(*dfd, sizeof(expected) + 4);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}

TEST_F(createDFDCompressedTest2, FormatETC2_SR8G8B8A8) {
    customize(KHR_DF_MODEL_ETC2, KHR_DF_PRIMARIES_BT709,
              KHR_DF_TRANSFER_SRGB, KHR_DF_FLAG_ALPHA_STRAIGHT,
              3, 3,
              {
                {0, 63, KHR_DF_CHANNEL_ETC2_ALPHA | KHR_DF_SAMPLE_DATATYPE_LINEAR,
                 0, 0, 0, 0, 0, 0xFFFFFFFF},
                {64, 63, KHR_DF_CHANNEL_ETC2_COLOR, 0, 0, 0, 0, 0, 0xFFFFFFFF},
              }
             );

    uint32_t* dfd = createDFDCompressed(c_ETC2_R8G8B8A8, 4, 4, s_SRGB);

    EXPECT_EQ(*dfd, sizeof(expected) + 4);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}

TEST_F(createDFDCompressedTest1x16, FormatASTC_12x12_SRGB) {
    customize(KHR_DF_MODEL_ASTC, KHR_DF_PRIMARIES_BT709,
              KHR_DF_TRANSFER_SRGB, KHR_DF_FLAG_ALPHA_STRAIGHT,
              11, 11,
              {
                {0, 127, KHR_DF_CHANNEL_ASTC_DATA, 0, 0, 0, 0, 0, 0xFFFFFFFF},
              }
             );

    uint32_t* dfd = createDFDCompressed(c_ASTC, 12, 12, s_SRGB);

    EXPECT_EQ(*dfd, sizeof(expected) + 4);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}

TEST_F(createDFDCompressedTest1x16, FormatASTC_10x5_SRGB) {
    customize(KHR_DF_MODEL_ASTC, KHR_DF_PRIMARIES_BT709,
              KHR_DF_TRANSFER_SRGB, KHR_DF_FLAG_ALPHA_STRAIGHT,
              9, 4,
              {
                {0, 127, KHR_DF_CHANNEL_ASTC_DATA, 0, 0, 0, 0, 0, 0xFFFFFFFF},
              }
             );

    uint32_t* dfd = createDFDCompressed(c_ASTC, 10, 5, s_SRGB);

    EXPECT_EQ(*dfd, sizeof(expected) + 4);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}

TEST_F(createDFDCompressedTest1x16, FormatASTC_5x4) {
    customize(KHR_DF_MODEL_ASTC, KHR_DF_PRIMARIES_BT709,
              KHR_DF_TRANSFER_LINEAR, KHR_DF_FLAG_ALPHA_STRAIGHT,
              4, 3,
              {
                {0, 127, KHR_DF_CHANNEL_ASTC_DATA, 0, 0, 0, 0, 0, 0xFFFFFFFF},
              }
             );

    uint32_t* dfd = createDFDCompressed(c_ASTC, 5, 4, s_UNORM);

    EXPECT_EQ(*dfd, sizeof(expected) + 4);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}

TEST_F(createDFDCompressedTest1x16, FormatASTC_10x8) {
    customize(KHR_DF_MODEL_ASTC, KHR_DF_PRIMARIES_BT709,
              KHR_DF_TRANSFER_LINEAR, KHR_DF_FLAG_ALPHA_STRAIGHT,
              9, 7,
              {
                {0, 127, KHR_DF_CHANNEL_ASTC_DATA, 0, 0, 0, 0, 0, 0xFFFFFFFF},
              }
             );

    uint32_t* dfd = createDFDCompressed(c_ASTC, 10, 8, s_UNORM);

    EXPECT_EQ(*dfd, sizeof(expected) + 4);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}

TEST_F(createDFDCompressedTest1, FormatBC1) {
    customize(KHR_DF_MODEL_BC1A, KHR_DF_PRIMARIES_BT709,
              KHR_DF_TRANSFER_LINEAR, KHR_DF_FLAG_ALPHA_STRAIGHT,
              3, 3,
              {
                {0, 63, KHR_DF_CHANNEL_BC1A_COLOR, 0, 0, 0, 0, 0, 0xFFFFFFFF},
              }
             );

    uint32_t* dfd = createDFDCompressed(c_BC1_RGB, 4, 4, s_UNORM);

    EXPECT_EQ(*dfd, sizeof(expected) + 4);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}

}  // namespace
