// Copyright (c) 2019 Andreas Atteneder, All Rights Reserved.

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#define OS_SEP '\\'
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

typedef struct {
    ktx_transcode_fmt_e format;
    bool supportsNonPo2;
    bool supportsNonAlpha;
} FormatFeature;

vector<TextureSet> allTextureSets = {
    {"color_grid_basis.ktx2","color_grid.basis",true,false},
    {"kodim17_basis.ktx2","kodim17.basis",false,false},
    {"alpha_simple_basis.ktx2","alpha_simple.basis",true,true}
};

vector<FormatFeature> allFormats = {
    {KTX_TTF_ETC1_RGB,true,true},
    {KTX_TTF_ETC2_RGBA,true,true},
    {KTX_TTF_BC1_RGB,true,true},
    {KTX_TTF_BC3_RGBA,true,true},
    {KTX_TTF_BC4_R,true,true},
    {KTX_TTF_BC5_RG,true,true},
    {KTX_TTF_BC7_M6_RGB,true,true},
    {KTX_TTF_BC7_M5_RGBA,true,true},
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
    {KTX_TTF_RGBA4444,true,true}
    // ATC and FXT1 formats are not supported by KTX2 as there
    // are no equivalent VkFormats.
};

class TextureCombinationsTest :
    public ::testing::TestWithParam<tuple<TextureSet,FormatFeature>> {};

INSTANTIATE_TEST_CASE_P(AllCombinations,
                        TextureCombinationsTest,
                        ::testing::Combine(::testing::ValuesIn(allTextureSets),
                                           ::testing::ValuesIn(allFormats)),);

bool read_file( string path, void** data, long *fsize ) {
    FILE *f = fopen(path.data(),"rb");
    if(f==NULL) {
        return false;
    }
    fseek(f, 0, SEEK_END);
    *fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

    *data = malloc(*fsize);
    fread(*data, 1, *fsize, f);
    fclose(f);
    return true;
}

bool isPo2(uint32_t i) {
    return (i&(i-1))==0;
}

string combine_paths(string const a, string const b) {
    if(a.back()==OS_SEP) {
        return a+b;
    } else {
        return a+OS_SEP+b;
    }
}

void test_texture_set( TextureSet & textureSet, FormatFeature & format ) {

    void * basisData;
    long basisSize;
    
    string path = combine_paths(image_path,textureSet.basisuPath);
    bool read_success = read_file(path, &basisData, &basisSize);

    ASSERT_TRUE(read_success) << "Could not open texture file " << path;

    basis_file basisu;

    basisu.open((uint8_t*)basisData, (uint32_t)basisSize);
    uint32_t bWidth = basisu.getImageWidth(0,0);
    uint32_t bHeight = basisu.getImageHeight(0,0);

    bool hasAlpha = basisu.getHasAlpha();

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

    void * data;
    long fsize;

    path = image_path+textureSet.ktxPath;
    read_success = read_file(path, &data, &fsize);

    ASSERT_TRUE(read_success);

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
    cout << "txt: " << ts.ktxPath
            << " format: " << format.format << "\n";
    test_texture_set(ts,format);
}
}  // namespace

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
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

    return RUN_ALL_TESTS();
}
