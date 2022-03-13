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
#include "gtest/gtest.h"
#include <vector>
#include <cstring>

#include "basisu_c_binding.h"

using namespace std;

string image_path;

namespace {

typedef struct {
    string ktxPath;
    string basisuPath;
    bool isPo2;
    bool hasAlpha;
} TextureSet;

std::ostream& operator<<(std::ostream& out, const TextureSet& h)
{
     return out << h.ktxPath;
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

vector<TextureSet> allTextureSets = {
    {"color_grid_basis.ktx2","color_grid.basis",true,false},
#if 1
    {"kodim17_basis.ktx2","kodim17.basis",false,false},
    {"alpha_simple_basis.ktx2","alpha_simple.basis",true,true}
#endif
};

vector<FormatFeature> allFormats = {
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
    public ::testing::TestWithParam<tuple<TextureSet,FormatFeature>> {};

INSTANTIATE_TEST_SUITE_P(AllCombinations,
                        TextureCombinationsTest,
                        ::testing::Combine(::testing::ValuesIn(allTextureSets),
                                           ::testing::ValuesIn(allFormats)));

bool read_file( string path, void** data, unsigned long *fsize ) {
    FILE *f = fopen(path.data(),"rb");
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

string combine_paths(string const a, string const b) {
	if (a.back() == OS_SEP) {
		return a + b;
#if defined(_WIN32)
	} else if (a.back() == UNIX_SEP) {
		return a + b;
#endif
	} else {
        return a+OS_SEP+b;
    }
}

void test_texture_set( TextureSet & textureSet, FormatFeature & format ) {

    void * basisData = nullptr;
    unsigned long basisSize = 0;
    
    string path = combine_paths(image_path,textureSet.basisuPath);
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

    path = combine_paths(image_path,textureSet.ktxPath);
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

    if(!::testing::FLAGS_gtest_list_tests) {
        if(argc!=2) {
            cerr << "Usage: " << argv[0] << " <test images path>\n";
            return -1;
        }

        image_path = string(argv[1]);

        struct stat info;

        if( stat( image_path.data(), &info ) != 0 ) {
            cerr << "Cannot access " << image_path << '\n';
            return -2;
        } else if( ! (info.st_mode & S_IFDIR) ) {
            cerr << image_path << "is not a valid directory\n";
            return -3;
        }
    }

    ktx_basisu_basis_init();

    return RUN_ALL_TESTS();
}
