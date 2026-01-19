/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2010-2020 Mark Callow, <khronos at callow dot im>.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file
 * @~English
 *
 * @brief Test multithreaded calls to ktxTexture2  functions.
 *
 * This is a smoke test for simultaneous calls from multiple threads to libktx encode, transcode
 * and decode functions. Use of multiple threads by the encoders and decoders themselves is
 * separately tested by their test suites.
 *
 * These tests need to run hundreds of times to have a chance of triggering a race, which takes
 * a long time. There is a cmake option to control how many times  CTest test will run this
 * program which defaults to 1.
 *
 * It is not clear how useful this test actually is.
 *
 * @author Mark Callow, github.com/MarkCallow
 */


#if defined(_WIN32)
  #define _CRT_SECURE_NO_WARNINGS
  #if _MSC_VER < 1900
    #define snprintf _snprintf
  #endif
#endif

#include <barrier>
#include <filesystem>
#include <thread>
#include <sys/stat.h>
#include "ktx.h"
#include "platform_utils.h"
#include "gtest/gtest.h"


namespace {

namespace fs = std::filesystem;

////////////////////////////////////////////
// Multithreaded basisu encode & transcode tests
///////////////////////////////////////////

fs::path ktx2Path;
//fs::path pngPath;
//fs::path goldenPath;

// Must be before any other test calling ktxTexture2_TranscodeBasis.
TEST(Multithreaded, TranscodeBasis) {
    const int numThreads = 2;
    std::barrier syncPoint(numThreads);

    auto funcLoad = [&syncPoint] (const std::u8string& ktxFile, const std::u8string& goldenFile) {
        ktxTexture2 *texture = nullptr;

        ktx_error_code_e result = ktxTexture_CreateFromNamedFile(
            reinterpret_cast<const char*>(ktxFile.c_str()),
            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, (ktxTexture **)&texture);
        ASSERT_TRUE(result == KTX_SUCCESS) << "ktxTexture_CreateFromNamedFile \""
                << from_u8string(ktxFile) << "\" failed: " << ktxErrorString(result);
        ASSERT_TRUE(texture != NULL) << "Returned texture pointer is NULL";
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";

        if (ktxTexture2_NeedsTranscoding(texture)) {
            auto targetFormat = KTX_TTF_ETC2_RGBA;

            // Barrier to make ktxTexture2_TranscodeBasis be called concurrently in multiple threads
            syncPoint.arrive_and_wait();

            result = ktxTexture2_TranscodeBasis(texture, targetFormat, 0);
            EXPECT_EQ(result, KTX_SUCCESS);
            //result = ktxTexture2_WriteToNamedFile(texture, "/tmp/testktx2");

            ktxTexture2* golden = nullptr;
            result = ktxTexture_CreateFromNamedFile(
                reinterpret_cast<const char*>(goldenFile.c_str()),
                KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, (ktxTexture **)&golden);
            ASSERT_TRUE(result == KTX_SUCCESS) << "ktxTexture_CreateFromNamedFile \""
                    << from_u8string(goldenFile) << "\" failed: " << ktxErrorString(result);
            ASSERT_TRUE(golden != NULL) << "Returned texture pointer is NULL";
            ASSERT_TRUE(golden->pData != NULL) << "Image data not loaded";
            EXPECT_EQ(texture->dataSize, golden->dataSize);
            EXPECT_EQ(memcmp(texture->pData, golden->pData, texture->dataSize), 0);
            ktxTexture2_Destroy(golden);
        }
        ktxTexture2_Destroy(texture);
    };

    fs::path ktxFile1 = ::ktx2Path;
    ktxFile1.replace_filename(u8"r8g8b8a8_srgb_mip_blze.ktx2");
    fs::path goldenFile1 = ::ktx2Path;
    goldenFile1.replace_filename(u8"r8g8b8a8_srgb_mip_etc2.ktx2");
    //fs::path image2 = ::ktx2Path;
    //image2.replace_filename(u8"kodim17_basis.ktx2");

    std::vector<std::thread> threads;
    threads.resize(numThreads);
    for (int i = 0; i < numThreads; i++) {
        threads[i] = std::thread([&funcLoad, ktxFile1, goldenFile1] {
            funcLoad(ktxFile1.u8string(), goldenFile1.u8string());
        });
    }

    for (int i = 0; i < numThreads; i++) {
        threads[i].join();
    }
}

TEST(Multithreaded, DecodeASTC) {
    const int numThreads = 2;
    std::barrier syncPoint(numThreads);

    auto funcLoad = [&syncPoint] (const std::u8string& ktxFile, const std::u8string& goldenFile) {
        ktxTexture2 *texture = nullptr;

        ktx_error_code_e result = ktxTexture_CreateFromNamedFile(
            reinterpret_cast<const char*>(ktxFile.c_str()),
            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, (ktxTexture **)&texture);
        ASSERT_TRUE(result == KTX_SUCCESS) << "ktxTexture_CreateFromNamedFile \""
                << from_u8string(ktxFile) << "\" failed: " << ktxErrorString(result);
        ASSERT_TRUE(texture != NULL) << "Returned texture pointer is NULL";
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";

        ASSERT_TRUE(ktxTexture2_GetColorModel_e(texture) == KHR_DF_MODEL_ASTC);

        // Barrier to make ktxTexture2_TranscodeBasis be called concurrently in multiple threads
        syncPoint.arrive_and_wait();

        result = ktxTexture2_DecodeAstc(texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        //result = ktxTexture2_WriteToNamedFile(texture, "/tmp/testktx2");

        ktxTexture2* golden = nullptr;
        result = ktxTexture_CreateFromNamedFile(
            reinterpret_cast<const char*>(goldenFile.c_str()),
            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, (ktxTexture **)&golden);
        ASSERT_TRUE(result == KTX_SUCCESS) << "ktxTexture_CreateFromNamedFile \""
                << from_u8string(goldenFile) << "\" failed: " << ktxErrorString(result);
        ASSERT_TRUE(golden != NULL) << "Returned texture pointer is NULL";
        ASSERT_TRUE(golden->pData != NULL) << "Image data not loaded";
        EXPECT_EQ(texture->dataSize, golden->dataSize);
        EXPECT_EQ(memcmp(texture->pData, golden->pData, texture->dataSize), 0);
        ktxTexture2_Destroy(golden);
        ktxTexture2_Destroy(texture);
    };

    fs::path ktxFile1 = ::ktx2Path;
    ktxFile1.replace_filename(u8"r8g8b8a8_srgb_mip_astc.ktx2");
    fs::path goldenFile1 = ::ktx2Path;
    goldenFile1.replace_filename(u8"r8g8b8a8_srgb_mip.ktx2");
    //fs::path image2 = ::ktx2Path;
    //image2.replace_filename(u8"kodim17_basis.ktx2");

    std::vector<std::thread> threads;
    threads.resize(numThreads);
    for (int i = 0; i < numThreads; i++) {
        threads[i] = std::thread([&funcLoad, ktxFile1, goldenFile1] {
            funcLoad(ktxFile1.u8string(), goldenFile1.u8string());
        });
    }

    for (int i = 0; i < numThreads; i++) {
        threads[i].join();
    }
}

class MultithreadedEncode : public ::testing::Test
{
protected:
    const unsigned int numThreads = 2;

public:
    void run(bool basisu) {
        std::barrier<> syncPoint(numThreads);

        auto funcEncode = [&syncPoint, basisu] (const std::u8string& imagePath) {
            ktxTexture2 *texture = nullptr;

            ktx_error_code_e result = ktxTexture_CreateFromNamedFile(
                    reinterpret_cast<const char*>(imagePath.c_str()),
                    KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, (ktxTexture **)&texture);
            // If imagePath contains non-ASCII unicode characters then, on
            // Windows, we have to tolerate mojibake in the error message as there
            // is no way to print utf-8 to std::cerr. Any hacks using fmt::print
            // result in the message order being wrong.
            ASSERT_TRUE(result == KTX_SUCCESS) << "ktxTexture_CreateFromNamedFile \""
                        << from_u8string(imagePath) << "\" failed: " << ktxErrorString(result);
            ASSERT_TRUE(texture != NULL) << "Returned texture pointer is NULL";
            ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";
            ASSERT_TRUE(!texture->isCompressed);

            // Barrier to make ktxTexture2_CompressBasis be called concurrently in multiple threads
            syncPoint.arrive_and_wait();

            if (basisu)
                result = ktxTexture2_CompressBasis(texture, 0);
            else
                result = ktxTexture2_CompressAstc(texture, 20);
            EXPECT_EQ(result, KTX_SUCCESS);

            ktxTexture2_Destroy(texture);
        };

        fs::path ktx2Input = ktx2Path;
        ktx2Input.replace_filename(u8"r8g8b8a8_srgb_mip.ktx2");

        std::vector<std::thread> threads;
        threads.resize(numThreads);
        for (unsigned int i = 0; i < numThreads; i++) {
            threads[i] = std::thread([&funcEncode, ktx2Input] {
                funcEncode(ktx2Input.u8string());
            });
        }

        for (unsigned int i = 0; i < numThreads; i++) {
            threads[i].join();
        }
    }
};

TEST_F(MultithreadedEncode, EncodeBasis) {
    run(true);
}

TEST_F(MultithreadedEncode, EncodeASTC) {
    run(false);
}

}  // namespace

GTEST_API_ int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);

    if (!::testing::FLAGS_gtest_list_tests) {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " <tests path>\n";
            return -1;
        }

        fs::path resourcesPath;
        std::vector<std::u8string> u8argv;
        InitUTF8CLI(argc, argv, u8argv);
        resourcesPath = u8argv[1];
        // Trailing / so path will be handled as directory.
        //goldenPath = resourcesPath / u8"golden/threadtests/";
        ktx2Path = resourcesPath / u8"ktx2/";

        auto checkPath = [](const fs::path path) {
            std::error_code ec;
            auto stat = fs::status(path, ec);
            if (!fs::exists(stat)) {
                std::cerr << std::format("{} does not exist.\n", from_u8string(path.u8string()));
                return -2;
            } else if (!fs::is_directory(stat)) {
                std::cerr << std::format("{} is not a directory.\n", from_u8string(path.u8string()));
                return -3;
            }
            return 0;
        };
        //std::cerr << std::format("test unicode name is {}.\n", "テクスチャー");
        // int ret = checkPath(goldenPath);
        // if (!ret)
        int ret = checkPath(ktx2Path);
        //if (!ret)
            // ret = checkPath(pngPath);
        if (ret) return ret;
    }

    return RUN_ALL_TESTS();
}
