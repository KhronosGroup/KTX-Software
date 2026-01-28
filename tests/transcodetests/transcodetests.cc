// Copyright 2019 Andreas Atteneder, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#define OS_SEP '\\'
#define UNIX_SEP '/'
#else
#define OS_SEP '/'
#endif

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "gl_format.h"
#include "ktx.h"
extern "C" {
  #include "ktxint.h"
  #include "filestream.h"
  #include "memstream.h"
}
#include "platform_utils.h"
#include "gtest/gtest.h"

#include <version>
#include <vector>
#include <cstring>
#include <filesystem>
#if defined(__cpp_lib_format)
  #include <format>
#else
  // Sigh!! gcc11 does not support std::format though it has a g++20 option.
  // Use {fmt} instead.
  #include <fmt/ostream.h>
#endif

#include "basisu_c_binding.h"

namespace {

namespace fs = std::filesystem;
#if defined(__cpp_lib_format)
  using namespace std;
#else
  using namespace fmt;
#endif

fs::path basisResources, ktxResources;

typedef struct {
    std::string ktxFile;
    std::string basisuFile;
    bool isPo2;
    bool hasAlpha;
} TextureSet;

std::ostream& operator<<(std::ostream& out, const TextureSet& h)
{
     return out << h.ktxFile;
}

typedef struct {
    ktx_transcode_fmt_e format;
    bool supportsNonPo2;
    bool supportsNonAlpha;
} FormatFeature;

std::ostream& operator<<(std::ostream& out, const FormatFeature& h)
{
     return out << ktxTranscodeFormatString(h.format);
}

std::vector<TextureSet> allTextureSets = {
    {"color_grid_blze.ktx2","color_grid.basis",true,false},
#if 1
    {"kodim17_blze.ktx2","kodim17.basis",false,false},
    {"alpha_simple_blze.ktx2","alpha_simple.basis",true,true}
#endif
};

std::vector<FormatFeature> allFormats = {
#if 1
    {KTX_TTF_ETC1_RGB,true,true},
    {KTX_TTF_ETC2_RGBA,true,true},
    {KTX_TTF_BC1_RGB,true,true},
    {KTX_TTF_BC3_RGBA,true,true},
    {KTX_TTF_BC4_R,true,true},
    {KTX_TTF_BC5_RG,true,true},
    {KTX_TTF_BC7_RGBA,true,true},
    {KTX_TTF_PVRTC1_4_RGB,false,true},
    {KTX_TTF_PVRTC1_4_RGBA,false,false},
    {KTX_TTF_ASTC_4x4_RGBA,true,true},
    {KTX_TTF_PVRTC2_4_RGB,true,true},
    {KTX_TTF_PVRTC2_4_RGBA,true,true},
    // {KTX_TTF_ETC2_EAC_R11,true,true},
    {KTX_TTF_ETC2_EAC_RG11,true,true},
    {KTX_TTF_RGBA32,true,true},
    {KTX_TTF_RGB565,true,true},
    {KTX_TTF_BGR565,true,true},
#endif
    {KTX_TTF_RGBA4444,true,true}
    // ATC and FXT1 formats are not supported by KTX2 as there
    // are no equivalent VkFormats.
};

class TextureCombinationsTest :
    public ::testing::TestWithParam<std::tuple<TextureSet,FormatFeature>> {};

INSTANTIATE_TEST_SUITE_P(AllCombinations,
                        TextureCombinationsTest,
                        ::testing::Combine(::testing::ValuesIn(allTextureSets),
                                           ::testing::ValuesIn(allFormats)));

bool read_file( fs::path file, void** data, unsigned long *fsize ) {
    FILE *f = fopenUTF8(file.u8string(), std::string("rb"));
    if(f==NULL) {
        return false;
    }
    fseek(f, 0, SEEK_END);
    *fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

    *data = malloc(*fsize);
    size_t numRead = fread(*data, 1, *fsize, f);
    fclose(f);
    return numRead == *fsize;
}

bool isPo2(uint32_t i) {
    return (i&(i-1))==0;
}

void test_texture_set( TextureSet & textureSet, FormatFeature & format ) {

    void * basisData = nullptr;
    unsigned long basisSize = 0;
    
    fs::path path = basisResources / textureSet.basisuFile;
    bool read_success = read_file(path, &basisData, &basisSize);

    ASSERT_TRUE(read_success) << "Could not open or read texture file " << path;

    basis_file basisu;

    basisu.open((uint8_t*)basisData, (uint32_t)basisSize);
    uint32_t bWidth = basisu.getImageWidth(0,0);
    uint32_t bHeight = basisu.getImageHeight(0,0);

    bool hasAlpha = basisu.getHasAlpha() > 0;

    ASSERT_EQ(hasAlpha,textureSet.hasAlpha);

    if( !hasAlpha && !format.supportsNonAlpha ) {
        return;
    }

    if(!(isPo2(bWidth) && isPo2(bHeight))
        && !format.supportsNonPo2 ) {
        return;
    }

    uint32_t finalSize = basisu.getImageTranscodedSizeInBytes(0,0,format.format);
    ktx_uint8_t* basisTranscodedData = (ktx_uint8_t*) malloc(finalSize);
    basisu.startTranscoding();
    uint32_t bRes = basisu.transcodeImage((void*)basisTranscodedData,finalSize,0,0,format.format,0,0);

    ASSERT_TRUE(bRes);

    basisu.close();

    void * data = 0; // = 0 to silence over-enthusiastic gcc 11 warning.
    unsigned long fsize;

    path = ktxResources / textureSet.ktxFile;
    read_success = read_file(path, &data, &fsize);

    ASSERT_TRUE(read_success) << "Could not open texture file " << path;

    KTX_error_code result;
    
    ktxTexture2* newTex = 0;
    
    result = ktxTexture2_CreateFromMemory(
        (const ktx_uint8_t*) data,
        (ktx_size_t) fsize,
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        (ktxTexture2**)&newTex
        );
    
    ASSERT_EQ(result,KTX_SUCCESS);

    result = ktxTexture2_TranscodeBasis(
                                        newTex,
                                        format.format,
                                        0
                                        );
    ASSERT_EQ(result,KTX_SUCCESS) << "Format " << format.format;

    EXPECT_EQ(bWidth,newTex->baseWidth);
    EXPECT_EQ(bHeight,newTex->baseHeight);
    EXPECT_EQ(finalSize,newTex->dataSize);

    int cmp = std::memcmp(basisTranscodedData,newTex->pData,finalSize);

    ASSERT_EQ(cmp,0);

    ktxTexture_Destroy(ktxTexture(newTex));

    free(data);
    free(basisTranscodedData);

    free(basisData);
}

TEST_P(TextureCombinationsTest, Basic) {
    TextureSet ts = get<0>(GetParam());
    FormatFeature format = get<1>(GetParam());
    test_texture_set(ts,format);
}
}  // namespace

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    if (!::testing::FLAGS_gtest_list_tests) {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " <test resources path>\n";
            return -1;
        }

        fs::path resourcesPath;
        std::vector<std::u8string> u8argv;
        InitUTF8CLI(argc, argv, u8argv);
        resourcesPath = u8argv[1];
        resourcesPath /= "";  // Ensure trailing / so path will be handled as a directory.

        std::error_code ec;
        auto stat = fs::status(resourcesPath, ec);
        if (!fs::exists(stat)) {
            std::cerr << format("{} does not exist.\n", from_u8string(resourcesPath.u8string()));
            return -2;
        } else if (!std::filesystem::is_directory(stat)) {
            std::cerr << format("{} is not a directory.\n",
                                from_u8string(resourcesPath.u8string()));
            return -3;
        }
        ktxResources = resourcesPath / u8"ktx2/";
        basisResources = resourcesPath / u8"basis/";
    }

    ktx_basisu_basis_init();

    return RUN_ALL_TESTS();
}
