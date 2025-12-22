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

    auto funcLoad = [&syncPoint] (const std::string& ktxFile, const std::string& goldenFile) {
        ktxTexture2 *texture = nullptr;

        KTX_error_code result = ktxTexture_CreateFromNamedFile(ktxFile.c_str(),
            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, (ktxTexture **)&texture);
        ASSERT_TRUE(result == KTX_SUCCESS) << "ktxTexture_CreateFromNamedFile \""
                << ktxFile << "\" failed: " << ktxErrorString(result);
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
            result = ktxTexture_CreateFromNamedFile(goldenFile.c_str(),
                KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, (ktxTexture **)&golden);
            ASSERT_TRUE(result == KTX_SUCCESS) << "ktxTexture_CreateFromNamedFile \""
                    << goldenFile << "\" failed: " << ktxErrorString(result);
            ASSERT_TRUE(golden != NULL) << "Returned texture pointer is NULL";
            ASSERT_TRUE(golden->pData != NULL) << "Image data not loaded";
            EXPECT_EQ(texture->dataSize, golden->dataSize);
            EXPECT_EQ(memcmp(texture->pData, golden->pData, texture->dataSize), 0);
            ktxTexture2_Destroy(golden);
        }
        ktxTexture2_Destroy(texture);
    };

    fs::path ktxFile1 = ::ktx2Path;
    ktxFile1.replace_filename(u8"rgba_mipmap_basis.ktx2");
    fs::path goldenFile1 = ::ktx2Path;
    goldenFile1.replace_filename(u8"rgba_mipmap_etc2.ktx2");
    //fs::path image2 = ::ktx2Path;
    //image2.replace_filename(u8"kodim17_basis.ktx2");

    std::vector<std::thread> threads;
    threads.resize(numThreads);
    for (int i = 0; i < numThreads; i++) {
        threads[i] = std::thread([&funcLoad, ktxFile1, goldenFile1] {
            funcLoad(ktxFile1.string(), goldenFile1.string());
        });
    }

    for (int i = 0; i < numThreads; i++) {
        threads[i].join();
    }
}

TEST(Multithreaded, DecodeASTC) {
    const int numThreads = 2;
    std::barrier syncPoint(numThreads);

    auto funcLoad = [&syncPoint] (const std::string& ktxFile, const std::string& goldenFile) {
        ktxTexture2 *texture = nullptr;

        KTX_error_code result = ktxTexture_CreateFromNamedFile(ktxFile.c_str(),
            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, (ktxTexture **)&texture);
        ASSERT_TRUE(result == KTX_SUCCESS) << "ktxTexture_CreateFromNamedFile \""
                << ktxFile << "\" failed: " << ktxErrorString(result);
        ASSERT_TRUE(texture != NULL) << "Returned texture pointer is NULL";
        ASSERT_TRUE(texture->pData != NULL) << "Image data not loaded";

        ASSERT_TRUE(ktxTexture2_GetColorModel_e(texture) == KHR_DF_MODEL_ASTC);

        // Barrier to make ktxTexture2_TranscodeBasis be called concurrently in multiple threads
        syncPoint.arrive_and_wait();

        result = ktxTexture2_DecodeAstc(texture);
        EXPECT_EQ(result, KTX_SUCCESS);
        //result = ktxTexture2_WriteToNamedFile(texture, "/tmp/testktx2");

        ktxTexture2* golden = nullptr;
        result = ktxTexture_CreateFromNamedFile(goldenFile.c_str(),
            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, (ktxTexture **)&golden);
        ASSERT_TRUE(result == KTX_SUCCESS) << "ktxTexture_CreateFromNamedFile \""
                << goldenFile << "\" failed: " << ktxErrorString(result);
        ASSERT_TRUE(golden != NULL) << "Returned texture pointer is NULL";
        ASSERT_TRUE(golden->pData != NULL) << "Image data not loaded";
        EXPECT_EQ(texture->dataSize, golden->dataSize);
        EXPECT_EQ(memcmp(texture->pData, golden->pData, texture->dataSize), 0);
        ktxTexture2_Destroy(golden);
        ktxTexture2_Destroy(texture);
    };

    fs::path ktxFile1 = ::ktx2Path;
    ktxFile1.replace_filename(u8"rgba_mipmap_astc.ktx2");
    fs::path goldenFile1 = ::ktx2Path;
    goldenFile1.replace_filename(u8"rgba_mipmap.ktx2");
    //fs::path image2 = ::ktx2Path;
    //image2.replace_filename(u8"kodim17_basis.ktx2");

    std::vector<std::thread> threads;
    threads.resize(numThreads);
    for (int i = 0; i < numThreads; i++) {
        threads[i] = std::thread([&funcLoad, ktxFile1, goldenFile1] {
            funcLoad(ktxFile1.string(), goldenFile1.string());
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

        auto funcEncode = [&syncPoint, basisu] (const std::string& imagePath) {
            ktxTexture2 *texture = nullptr;

            KTX_error_code result = ktxTexture_CreateFromNamedFile(
                    imagePath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, (ktxTexture **)&texture);
            ASSERT_TRUE(result == KTX_SUCCESS) << "ktxTexture_CreateFromNamedFile \""
                    << imagePath << "\" failed: " << ktxErrorString(result);
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

        fs::path ktx2Input = ::ktx2Path;
        ktx2Input.replace_filename(u8"rgba_mipmap.ktx2");

        std::vector<std::thread> threads;
        threads.resize(numThreads);
        for (unsigned int i = 0; i < numThreads; i++) {
          threads[i] = std::thread([&funcEncode, ktx2Input] {
                funcEncode(ktx2Input.string());
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

#if defined(_WIN32)
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
// For Windows, we convert the UTF-8 path to a UTF-16 path to force using
// the APIs that correctly handle unicode characters.
inline std::wstring
DecodeUTF8Path(const std::u8string& path) {
    std::wstring result;
    int len =
        MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(path.c_str()),
                            static_cast<int>(path.length()), NULL, 0);
    if (len > 0) {
        result.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(path.c_str()),
                            static_cast<int>(path.length()), &result[0], len);
    }
    return result;
}
#else
// For other platforms there is no need for any conversion, they
// support UTF-8 natively.
inline std::u8string DecodeUTF8Path(std::u8string path) { return path; }
#endif

#if defined(WIN32)
  #define stat _stat64i32
#endif

static int
statUTF8(const std::u8string& path, struct stat* info) {
#if defined(_WIN32)
    return _wstat(DecodeUTF8Path(path).c_str(), info);
#else
    return stat(reinterpret_cast<const char*>(path.c_str()), info);
#endif
}

GTEST_API_ int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);

    if (!::testing::FLAGS_gtest_list_tests) {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " <tests path>\n";
            return -1;
        }

        fs::path resourcesPath;
#if defined(_WIN32)
        // Manually acquire the wide char command line in case a unicode
        // filename has been specified.
        int allargc;
        LPWSTR commandLine = GetCommandLineW();
        LPWSTR* wideArgv = CommandLineToArgvW(commandLine, &allargc);
        // commandLine still has all the arguments including those removed
        // by InitGoogleTest, hence the arg index calculation.
        resourcesPath = wideArgv[allargc - argc + 1];
#else
        resourcesPath = argv[1];
#endif
        // Trailing / so path will be handled as directory.
        //goldenPath = resourcesPath / u8"golden/threadtests/";
        ktx2Path = resourcesPath / u8"input/ktx2/";
        //pngPath = resourcesPath / u8"input/png/";

        auto checkPath = [] (const fs::path path) {
            struct stat info;
            if (statUTF8(path.u8string().c_str(), &info) != 0) {
                std::cerr << "Cannot access " << path << std::endl;
                return -2;
            }  else if (!(info.st_mode & S_IFDIR)) {
                std::cerr << path << " is not a valid directory\n";
                return -3;
            }
            return 0;
        };
        //checkPath(goldenPath);
        checkPath(ktx2Path);
        //checkPath(pngPath);
    }

    return RUN_ALL_TESTS();
}
