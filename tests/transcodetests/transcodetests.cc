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
#include "vkformat_enum.h"
#include "gtest/gtest.h"

#include <version>
#include <vector>
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <memory>
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
        free(basisData);
        return;
    }

    if(!(isPo2(bWidth) && isPo2(bHeight))
        && !format.supportsNonPo2 ) {
        free(basisData);
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

//////////////////////////////
// UASTC HDR 6x6 intermediate SGD image description indexing
//////////////////////////////

// The image description table in the supercompression global data is written
// in level order, level 0 first, each level contributing
// numLayers * numFaces * depth(level) descriptions. For 3D textures depth
// halves with each level, so indexing the table with
// level * levelImageCount selected descriptions belonging to other levels
// and transcoding failed. The 2D case covers the constant-image-count path.

static ktx_uint16_t
floatToHalf(float value) {
    ktx_uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    const ktx_uint32_t sign = (bits >> 16) & 0x8000u;
    const ktx_int32_t exponent = (ktx_int32_t)((bits >> 23) & 0xFFu) - 127 + 15;
    const ktx_uint32_t mantissa = (bits >> 13) & 0x3FFu;
    if (exponent <= 0)
        return (ktx_uint16_t)sign;
    if (exponent >= 31)
        return (ktx_uint16_t)(sign | 0x7C00u);
    return (ktx_uint16_t)(sign | ((ktx_uint32_t)exponent << 10) | mantissa);
}

static void
roundTripUastcHdr6x6i(ktx_uint32_t numDimensions) {
    ktxTextureCreateInfo createInfo = {};
    createInfo.vkFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    createInfo.baseWidth = 24;
    createInfo.baseHeight = 24;
    createInfo.baseDepth = numDimensions == 3 ? 8 : 1;
    createInfo.numDimensions = numDimensions;
    createInfo.numLevels = 4;
    createInfo.numLayers = 1;
    createInfo.numFaces = 1;
    createInfo.isArray = KTX_FALSE;
    createInfo.generateMipmaps = KTX_FALSE;

    ktxTexture2* texture = nullptr;
    KTX_error_code result = ktxTexture2_Create(&createInfo,
                                              KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                              &texture);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    std::unique_ptr<ktxTexture2, void(*)(ktxTexture2*)> texture_raii(
        texture, [](ktxTexture2* t) { ktxTexture_Destroy(ktxTexture(t)); });

    for (ktx_uint32_t level = 0; level < createInfo.numLevels; level++) {
        const ktx_uint32_t width = std::max(1u, createInfo.baseWidth >> level);
        const ktx_uint32_t height = std::max(1u, createInfo.baseHeight >> level);
        const ktx_uint32_t depth = std::max(1u, createInfo.baseDepth >> level);
        for (ktx_uint32_t slice = 0; slice < depth; slice++) {
            std::vector<ktx_uint16_t> pixels((size_t)width * height * 4);
            for (ktx_uint32_t y = 0; y < height; y++) {
                for (ktx_uint32_t x = 0; x < width; x++) {
                    const size_t i = ((size_t)y * width + x) * 4;
                    pixels[i + 0] = floatToHalf(0.1f + 2.0f * x / width + level);
                    pixels[i + 1] = floatToHalf(0.2f + 1.5f * y / height + slice);
                    pixels[i + 2] = floatToHalf(0.4f + 0.5f * level);
                    pixels[i + 3] = floatToHalf(1.0f);
                }
            }
            result = ktxTexture_SetImageFromMemory(
                         ktxTexture(texture), level, 0, slice,
                         reinterpret_cast<const ktx_uint8_t*>(pixels.data()),
                         pixels.size() * sizeof(ktx_uint16_t));
            ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
        }
    }

    ktxBasisParams cparams = {};
    cparams.structSize = sizeof(cparams);
    cparams.threadCount = 1;
    cparams.codec = KTX_BASIS_CODEC_UASTC_HDR_6x6_INTERMEDIATE;
    result = ktxTexture2_CompressBasisEx(texture, &cparams);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    ASSERT_EQ(texture->supercompressionScheme, KTX_SS_UASTC_HDR_6x6_INTERMEDIATE);

    result = ktxTexture2_TranscodeBasis(texture, KTX_TTF_ASTC_HDR_6x6_RGBA, 0);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    EXPECT_EQ(texture->vkFormat,
              static_cast<ktx_uint32_t>(VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK));
    EXPECT_EQ(texture->supercompressionScheme, KTX_SS_NONE);
    EXPECT_NE(texture->pData, nullptr);
}

TEST(TranscodeUastcHdr6x6i, RoundTrip2D) {
    roundTripUastcHdr6x6i(2);
}

TEST(TranscodeUastcHdr6x6i, RoundTrip3DMipLevels) {
    roundTripUastcHdr6x6i(3);
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
