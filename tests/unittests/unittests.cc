/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2010-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
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
#include "ltexceptions.h"

// These are so we can test swizzle_to_rgba. Do not put inside "namespace {"
// as there is no "namespace {" in basis_encode.cpp where the function is
// defined.
enum swizzle_e {
    R = 1,
    G = 2,
    B = 3,
    A = 4,
    ZERO = 5,
    ONE = 6,
};

extern void
swizzle_to_rgba(uint8_t* rgbadst, uint8_t* rgbasrc, uint32_t src_len,
                  ktx_size_t image_size, swizzle_e swizzle[4]);

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
    EXPECT_EQ(helper.images.size(), 1U);
    EXPECT_EQ(helper.images[0].size(), 1U);
    EXPECT_EQ(helper.images[0][0].size(), 1U);
    EXPECT_EQ(helper.images[0][0][0].size(), 32 * 32 * 3U);
    EXPECT_EQ(numComponents(), 3U);
}

TEST_F(WriterTestHelperRGB8Test, Construct3D) {
    helper.resize(createFlagBits::eNone, 1, 1, 3, 32, 32, 32);
    EXPECT_EQ(helper.images.size(), 1U);
    EXPECT_EQ(helper.images[0].size(), 1U);
    EXPECT_EQ(helper.images[0][0].size(), 32U);
    EXPECT_EQ(helper.images[0][0][0].size(), (size_t)(32 * 32 * 3));
    EXPECT_EQ(numComponents(), 3U);
}

/////////////////////////////////////
// Base fixture for createDFD tests.
/////////////////////////////////////

// TODO: Move DFD tests to dfdutils when build is redone with CMake.

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

    EXPECT_EQ(*dfd, sizeof(expected) + 4U);
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

    EXPECT_EQ(*dfd, sizeof(expected) + 4U);
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

    EXPECT_EQ(*dfd, sizeof(expected) + 4U);
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

    EXPECT_EQ(*dfd, sizeof(expected) + 4U);
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

    EXPECT_EQ(*dfd, sizeof(expected) + 4U);
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

    EXPECT_EQ(*dfd, sizeof(expected) + 4U);
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

    EXPECT_EQ(*dfd, sizeof(expected) + 4U);
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

    EXPECT_EQ(*dfd, sizeof(expected) + 4U);
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

    EXPECT_EQ(*dfd, sizeof(expected) + 4U);
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

    EXPECT_EQ(*dfd, sizeof(expected) + 4U);
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

    EXPECT_EQ(*dfd, sizeof(expected) + 4U);
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

    EXPECT_EQ(*dfd, sizeof(expected) + 4U);
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

    EXPECT_EQ(*dfd, sizeof(expected) + 4U);
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

    EXPECT_EQ(*dfd, sizeof(expected) + 4U);
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

    EXPECT_EQ(*dfd, sizeof(expected) + 4U);
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

    EXPECT_EQ(*dfd, sizeof(expected) + 4U);
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

    EXPECT_EQ(*dfd, sizeof(expected) + 4U);
    EXPECT_EQ(memcmp(&expected, dfd+1, sizeof(expected)), 0);

    free(dfd);
}

class DFDVkFormatListTest : public ::testing::Test {
protected:
    DFDVkFormatListTest() {}

    static constexpr VkFormat formats[] = {
        #include "vkformat_list.inl"
    };
};

extern "C" const char* vkFormatString(VkFormat format);

TEST_F(DFDVkFormatListTest, ReconstructDFDBytesPlane0) {

    for (uint32_t i = 0; i < sizeof(formats) / sizeof(VkFormat); i++) {
        uint32_t* dfd = vk2dfd(formats[i]);
        ASSERT_TRUE(dfd != NULL) << "vk2dfd failed to produce DFD for "
                                 << vkFormatString(formats[i]);
        uint32_t* bdfd = dfd + 1;
        uint32_t origBytesPlane0 = KHR_DFDVAL(bdfd, BYTESPLANE0);
        KHR_DFDSETVAL(bdfd, BYTESPLANE0, 0);
        uint32_t reconstructedBytesPlane0 = reconstructDFDBytesPlane0FromSamples(dfd);
        EXPECT_EQ(origBytesPlane0, reconstructedBytesPlane0);
        free(dfd);
    }
}

TEST_F(DFDVkFormatListTest, BidirectionalVk2DfDTest) {

    for (uint32_t i = 0; i < sizeof(formats) / sizeof(VkFormat); i++) {
        uint32_t* dfd = vk2dfd(formats[i]);
        ASSERT_TRUE(dfd != NULL) << "vk2dfd failed to produce DFD for "
                                 << vkFormatString(formats[i]);
        VkFormat formatOut = dfd2vk(dfd);
        // The SCALED formats are indistinguishable from the INT formats
        // and dfd2vk resolves the ambiguity in favor of the format more
        // likely to be used as a texture.
        //
        // The A8B8G8R8_*_PACK32 formats are indistinguishable from the
        // R8G8B8A8* formats and dfd2vk returns the more common format.
        switch (formats[i]) {
          case VK_FORMAT_R8_USCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_R8_UINT);
            break;
          case VK_FORMAT_R8_SSCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_R8_SINT);
            break;
          case VK_FORMAT_R8G8_USCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_R8G8_UINT);
            break;
          case VK_FORMAT_R8G8_SSCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_R8G8_SINT);
            break;
          case VK_FORMAT_B8G8R8_USCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_B8G8R8_UINT);
            break;
          case VK_FORMAT_B8G8R8_SSCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_B8G8R8_SINT);
            break;
          case VK_FORMAT_R8G8B8_USCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_R8G8B8_UINT);
            break;
          case VK_FORMAT_R8G8B8_SSCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_R8G8B8_SINT);
            break;
          case VK_FORMAT_R8G8B8A8_USCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_R8G8B8A8_UINT);
            break;
          case VK_FORMAT_R8G8B8A8_SSCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_R8G8B8A8_SINT);
            break;
          case VK_FORMAT_B8G8R8A8_USCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_B8G8R8A8_UINT);
            break;
          case VK_FORMAT_B8G8R8A8_SSCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_B8G8R8A8_SINT);
            break;
          case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
            EXPECT_EQ(formatOut, VK_FORMAT_R8G8B8A8_UINT);
            break;
          case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
            EXPECT_EQ(formatOut, VK_FORMAT_R8G8B8A8_SINT);
            break;
          case VK_FORMAT_A8B8G8R8_UINT_PACK32:
            EXPECT_EQ(formatOut, VK_FORMAT_R8G8B8A8_UINT);
            break;
          case VK_FORMAT_A8B8G8R8_SINT_PACK32:
            EXPECT_EQ(formatOut, VK_FORMAT_R8G8B8A8_SINT);
            break;
          case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
            EXPECT_EQ(formatOut, VK_FORMAT_R8G8B8A8_SRGB);
            break;
          case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
            EXPECT_EQ(formatOut, VK_FORMAT_A2R10G10B10_UINT_PACK32);
            break;
          case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
            EXPECT_EQ(formatOut, VK_FORMAT_A2R10G10B10_SINT_PACK32);
            break;
          case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
            EXPECT_EQ(formatOut, VK_FORMAT_A2B10G10R10_UINT_PACK32);
            break;
          case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
            EXPECT_EQ(formatOut, VK_FORMAT_A2B10G10R10_SINT_PACK32);
            break;
          case VK_FORMAT_R16_USCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_R16_UINT);
            break;
          case VK_FORMAT_R16_SSCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_R16_SINT);
            break;
          case VK_FORMAT_R16G16_USCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_R16G16_UINT);
            break;
          case VK_FORMAT_R16G16_SSCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_R16G16_SINT);
            break;
          case VK_FORMAT_R16G16B16_USCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_R16G16B16_UINT);
            break;
          case VK_FORMAT_R16G16B16_SSCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_R16G16B16_SINT);
            break;
          case VK_FORMAT_R16G16B16A16_USCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_R16G16B16A16_UINT);
            break;
          case VK_FORMAT_R16G16B16A16_SSCALED:
            EXPECT_EQ(formatOut, VK_FORMAT_R16G16B16A16_SINT);
            break;
          case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
            EXPECT_EQ(formatOut, VK_FORMAT_R8G8B8A8_UNORM);
            break;
          case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
            EXPECT_EQ(formatOut, VK_FORMAT_R8G8B8A8_SNORM);
            break;
          default:
            EXPECT_EQ(formatOut, formats[i]);
        }
        free(dfd);
    }
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

    void compareList(ktxHashList list, bool isSorted) {
        ktxHashListEntry* entry = list;
        ktx_uint32_t entryCount = 0;

        for (; entry != NULL; entry = ktxHashList_Next(entry)) {
            char* key;
            ktx_uint8_t* value;
            ktx_uint32_t keyLen, valueLen;

            entryCount++;
            ktxHashListEntry_GetKey(entry, &keyLen, &key);
            ktxHashListEntry_GetValue(entry, &valueLen, (void**)&value);
            if (isSorted) {
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
        EXPECT_EQ(entryCount, 2U);
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

///////////////////////
// Swizzle test fixture
///////////////////////

template<ktx_uint32_t num_components, GLenum internalformat>
class SwizzleTestBase : public ::testing::Test {
  public:
    SwizzleTestBase() {
        std::vector<GLubyte> color;
        // Use swizzle enumerator values for easy checking of result.
        std::vector<GLubyte> defaultColor = { R, G, B, A };
        color.resize(num_components);
        for (uint32_t i = 0; i < num_components; i++) {
            color[i] = defaultColor[i];
        }
        helper.resize(createFlagBits::eNone, 1, 1, 2, width, height, 1, &color);
    }

    void runTest(swizzle_e swizzle[4]) {
        ktxTexture2* texture;
        ktx_error_code_e result;

        helper.texinfo.vkFormat
                = vkGetFormatFromOpenGLInternalFormat(helper.texinfo.glInternalformat);
        result = ktxTexture2_Create(&helper.texinfo,
                                    KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                    &texture);
        ASSERT_TRUE(result == KTX_SUCCESS);
        ASSERT_TRUE(texture != NULL) << "ktxTexture_CreateFromMemory failed: "
                                     << ktxErrorString(result);
        ASSERT_TRUE(texture->pData != NULL) << "Image stoage not allocated";

        result = helper.copyImagesToTexture(ktxTexture(texture));
        ASSERT_TRUE(result == KTX_SUCCESS);

        size_t destByteLen = width * height * 4 * sizeof(GLubyte);
        size_t srcByteLen = width * height * num_components * sizeof(GLubyte);
        typedef GLubyte color[4];
        color* dest = (color*)malloc(destByteLen);
        memset(dest, 0x7f, destByteLen);
        swizzle_to_rgba((uint8_t*)dest,
                        texture->pData,
                        num_components,
                        srcByteLen,
                        swizzle);

       for (uint32_t i = 0; i < width * height; i++) {
           for (uint32_t c = 0; c < 4; c++) {
               if (swizzle[c] == ZERO)
                   EXPECT_EQ(dest[i][c], 0);
               else if (swizzle[c] == ONE)
                   EXPECT_EQ(dest[i][c], 255);
               else
                   EXPECT_EQ(swizzle[c], dest[i][c]) << "c = " << c << ", i = "  << i;
           }
       }
    }

  protected:
    WriterTestHelper<GLubyte, num_components, internalformat> helper;
    const unsigned int width = 16;
    const unsigned int height = 16;
};

class SwizzleToRGBATestR8 : public SwizzleTestBase<1, GL_R8> { };
class SwizzleToRGBATestRG8 : public SwizzleTestBase<2, GL_RG8> { };
class SwizzleToRGBATestRGB8 : public SwizzleTestBase<3, GL_RGB8> { };
class SwizzleToRGBATestRGBA8 : public SwizzleTestBase<4, GL_RGBA8> { };

////////////////////////
// swizzle_to_rgba tests
////////////////////////

TEST_F(SwizzleToRGBATestR8, RRRONE) {
    swizzle_e r_to_rgba_mapping[4] = { R, R, R, ONE };
    runTest(r_to_rgba_mapping);
}

TEST_F(SwizzleToRGBATestRG8, RRRG) {
    swizzle_e r_to_rgba_mapping[4] = { R, R, R, G };
    runTest(r_to_rgba_mapping);
}

TEST_F(SwizzleToRGBATestRGB8, RGBONE) {
    swizzle_e r_to_rgba_mapping[4] = { R, G, B, ONE };
    runTest(r_to_rgba_mapping);
}

TEST_F(SwizzleToRGBATestRGB8, RRRG) {
    swizzle_e r_to_rgba_mapping[4] = { R, R, R, G };
    runTest(r_to_rgba_mapping);
}

TEST_F(SwizzleToRGBATestRGBA8, RGBA) {
    swizzle_e r_to_rgba_mapping[4] = { R, G, B, A };
    runTest(r_to_rgba_mapping);
}

TEST_F(SwizzleToRGBATestRGBA8, RRRG) {
    swizzle_e r_to_rgba_mapping[4] = { R, R, R, G };
    runTest(r_to_rgba_mapping);
}

TEST_F(SwizzleToRGBATestRGBA8, BGRA) {
    swizzle_e r_to_rgba_mapping[4] = { B, G, R, A };
    runTest(r_to_rgba_mapping);
}

TEST_F(SwizzleToRGBATestRGBA8, BGRZERO) {
    swizzle_e r_to_rgba_mapping[4] = { B, G, R, ZERO };
    runTest(r_to_rgba_mapping);
}

TEST_F(SwizzleToRGBATestRGBA8, ARGB) {
    swizzle_e r_to_rgba_mapping[4] = { A, R, G, B };
    runTest(r_to_rgba_mapping);
}

//////////////////////////////
// LoadTest exceptions tests
//////////////////////////////

#define OUT_OF_HOST_MEMORY -1
#define OUT_OF_DEVICE_MEMORY -2
#define FRAGMENTED_POOL -12
#define OUT_OF_POOL_MEMORY -1000069000

TEST(BadVulkanAllocExceptionTest, NoDeviceMemory) {
    try {
        throw bad_vulkan_alloc(OUT_OF_DEVICE_MEMORY, "no device memory test");
    } catch (bad_vulkan_alloc& e) {
        EXPECT_EQ(strcmp(e.what(), "Out of device memory for no device memory test."), 0);
    }
}

TEST(BadVulkanAllocExceptionTest, NoHostMemory) {
    try {
        throw bad_vulkan_alloc(OUT_OF_HOST_MEMORY, "no host memory test");
    } catch (bad_vulkan_alloc& e) {
        EXPECT_EQ(strcmp(e.what(), "Out of host memory for no host memory test."), 0);
    }
}

TEST(BadVulkanAllocExceptionTest, NoPoolMemory) {
    try {
        throw bad_vulkan_alloc(OUT_OF_POOL_MEMORY, "no pool memory test");
    } catch (bad_vulkan_alloc& e) {
        EXPECT_EQ(strcmp(e.what(), "Out of pool memory for no pool memory test."), 0);
    }
}

TEST(BadVulkanAllocExceptionTest, PoolFragmented) {
    try {
        throw bad_vulkan_alloc(FRAGMENTED_POOL, "fragmented pool memory test");
    } catch (bad_vulkan_alloc& e) {
        EXPECT_EQ(strcmp(e.what(), "Pool fragmented when allocating for fragmented pool memory test."), 0);
    }
}

}  // namespace
