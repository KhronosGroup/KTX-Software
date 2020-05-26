/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

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

class CheckHeader1Test : public ::testing::Test {
  protected:
    CheckHeader1Test() {
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
        testHeader.numberOfMipLevels = 5;
        testHeader.bytesOfKeyValueData = 0;
    }

    KTX_header testHeader;
  public:
    static ktx_uint8_t ktxId[12];
};

ktx_uint8_t CheckHeader1Test::ktxId[12] = {
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

//////////////////////////////
// CheckHeaderTest
//////////////////////////////

#if defined(DEBUG)
TEST_F(CheckHeader1Test, AssertsOnNullArguments) {
    ASSERT_DEATH_IF_SUPPORTED(ktxCheckHeader1_(0, 0), "Assert*");
}
#endif

TEST_F(CheckHeader1Test, ValidatesIdentifier) {
    KTX_supplemental_info suppInfo;

    EXPECT_EQ(ktxCheckHeader1_(&testHeader, &suppInfo), KTX_SUCCESS);

    testHeader.identifier[9] = 0;
    EXPECT_EQ(ktxCheckHeader1_(&testHeader, &suppInfo), KTX_UNKNOWN_FILE_FORMAT);
}

TEST_F(CheckHeader1Test, DisallowsInvalidEndianness) {
    KTX_supplemental_info suppInfo;

    testHeader.endianness = 0;
    EXPECT_EQ(ktxCheckHeader1_(&testHeader, &suppInfo), KTX_FILE_DATA_ERROR);
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

/////////////////////////////////////
// Base fixture for createDFD tests.
/////////////////////////////////////

#include <KHR/khr_df.h>
#define LIBKTX // To make dfd.h not include vulkan/vulkan_core.h.
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

    void customize(ktx_uint8_t model,
                   ktx_uint8_t primaries, ktx_uint8_t transfer,
                   ktx_uint8_t flags,
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
       expected.texelBlockDimension3 = 0;
    }

    void customize(ktx_uint8_t model,
                   ktx_uint8_t primaries, ktx_uint8_t transfer,
                   ktx_uint8_t flags,
                   ktx_uint32_t dim0, ktx_uint32_t dim1, ktx_uint32_t dim2,
                   std::initializer_list<sampleType> samples) {
        expected.model = model;
        expected.primaries = primaries;
        expected.transfer = transfer;
        expected.flags = flags;
        expected.texelBlockDimension0 = dim0;
        expected.texelBlockDimension1 = dim1;
        expected.texelBlockDimension2 = dim2;

        uint32_t i = 0; // There's got to be some syntax for declaring this in the for
        for (auto sample : samples ) {
            expected.samples[i] = sample;
            if (++i == numComponents)
                break;
        }
    }

    void customize(ktx_uint8_t model,
               ktx_uint8_t primaries, ktx_uint8_t transfer,
               ktx_uint8_t flags, ktx_uint32_t dim0, ktx_uint32_t dim1,
               std::initializer_list<sampleType> samples) {
        customize(model, primaries, transfer, flags, dim0, dim1, 0, samples);
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
                {0, 63, KHR_DF_CHANNEL_ETC1S_RGB , 0, 0, 0, 0, 0, 0xFFFFFFFF},
              }
             );

    uint32_t* dfd = createDFDCompressed(c_ETC1S, 4, 4, 1, s_UNORM);

    EXPECT_EQ(*dfd, sizeof(expected) + 4);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}

TEST_F(createDFDCompressedTest1, FormatETC1S_SR8B8G8) {
    customize(KHR_DF_MODEL_ETC1S, KHR_DF_PRIMARIES_BT709,
              KHR_DF_TRANSFER_SRGB, KHR_DF_FLAG_ALPHA_STRAIGHT,
              3, 3,
              {
                {0, 63, KHR_DF_CHANNEL_ETC1S_RGB, 0, 0, 0, 0, 0, 0xFFFFFFFF},
              }
             );

    uint32_t* dfd = createDFDCompressed(c_ETC1S, 4, 4, 1, s_SRGB);

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

    uint32_t* dfd = createDFDCompressed(c_ETC2_R8G8B8, 4, 4, 1, s_UNORM);

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

    uint32_t* dfd = createDFDCompressed(c_ETC2_R8G8B8A8, 4, 4, 1, s_UNORM);

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

    uint32_t* dfd = createDFDCompressed(c_ETC2_R8G8B8, 4, 4, 1, s_SRGB);

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

    uint32_t* dfd = createDFDCompressed(c_ETC2_R8G8B8A8, 4, 4, 1, s_SRGB);

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

    uint32_t* dfd = createDFDCompressed(c_ASTC, 12, 12, 1, s_SRGB);

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

    uint32_t* dfd = createDFDCompressed(c_ASTC, 10, 5, 1, s_SRGB);

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

    uint32_t* dfd = createDFDCompressed(c_ASTC, 5, 4, 1, s_UNORM);

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

    uint32_t* dfd = createDFDCompressed(c_ASTC, 10, 8, 1, s_UNORM);

    EXPECT_EQ(*dfd, sizeof(expected) + 4);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}

TEST_F(createDFDCompressedTest1x16, FormatASTC_3x3x3) {
    customize(KHR_DF_MODEL_ASTC, KHR_DF_PRIMARIES_BT709,
              KHR_DF_TRANSFER_LINEAR, KHR_DF_FLAG_ALPHA_STRAIGHT,
              2, 2, 2,
              {
                {0, 127, KHR_DF_CHANNEL_ASTC_DATA, 0, 0, 0, 0, 0, 0xFFFFFFFF},
              }
             );

    uint32_t* dfd = createDFDCompressed(c_ASTC, 3, 3, 3, s_UNORM);

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

    uint32_t* dfd = createDFDCompressed(c_BC1_RGB, 4, 4, 1, s_UNORM);

    EXPECT_EQ(*dfd, sizeof(expected) + 4);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}




//////////////////////////////
// HashListTest Fixture
//////////////////////////////

class HashListTest : public ::testing::Test {
  protected:
    HashListTest() : writerVal("HashListTest"), orientationVal("ruo") { }

    void constructList(bool sort) {
        KTX_error_code result;

        ktxHashList_Construct(&head);
        result = ktxHashList_AddKVPair(&head, KTX_WRITER_KEY,
                                       (ktx_uint32_t)writerVal.length() + 1,
                                       writerVal.c_str());
        EXPECT_EQ(result, KTX_SUCCESS);
        result = ktxHashList_AddKVPair(&head, KTX_ORIENTATION_KEY,
                                       (ktx_uint32_t)orientationVal.length() + 1,
                                       orientationVal.c_str());
        EXPECT_EQ(result, KTX_SUCCESS);
        if (sort) {
            ktxHashList_Sort(&head);
            sorted = true;
        }
    }

    void checkList() {
        compareList(head, sorted);
    }

    void compareList(ktxHashList list, bool sorted) {
        ktxHashListEntry* entry = list;
        ktx_uint32_t entryCount = 0;

        for (; entry != NULL; entry = ktxHashList_Next(entry)) {
            char* key;
            ktx_uint8_t* value;
            ktx_uint32_t keyLen, valueLen;

            entryCount++;
            ktxHashListEntry_GetKey(entry, &keyLen, &key);
            ktxHashListEntry_GetValue(entry, &valueLen, (void**)&value);
            if (sorted) {
                switch (entryCount) {
                  case 1:
                    EXPECT_STREQ(key, KTX_ORIENTATION_KEY);
                    break;
                  case 2:
                    EXPECT_STREQ(key, KTX_WRITER_KEY);
                    break;
                  default:
                    break;
                }
            }
            if (strcmp(key, KTX_ORIENTATION_KEY) == 0)
                EXPECT_EQ(orientationVal.compare((char*)value), 0);
            else if (strcmp(key, KTX_WRITER_KEY) == 0)
                EXPECT_EQ(writerVal.compare((char*)value), 0);
            else
                EXPECT_TRUE(false);
        }
        EXPECT_EQ(entryCount, 2);
    }

    ktxHashList head;
    std::string writerVal;
    std::string orientationVal;
    bool sorted;
};


//////////////////////////////
// HashListTests
//////////////////////////////


TEST_F(HashListTest, ConstructSorted) {
    constructList(true);
    checkList();
}

TEST_F(HashListTest, ConstructCopy) {
    ktxHashList copyHead;

    constructList(true);
    ktxHashList_ConstructCopy(&copyHead, head);
    compareList(copyHead, true);
}


}  // namespace
