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


#include <string.h>
#include "gl_format.h"
#include "ktx.h"
extern "C" {
  #include "ktxint.h"
  #include "filestream.h"
  #include "memstream.h"
}
#include "gtest/gtest.h"
#include <vector>

#include "basisu_c_binding.h"

using namespace std;

string image_path;

namespace {

typedef struct {
    string ktxPath;
    string basisuPath;
} TextureSet;

vector<TextureSet> allTextureSets = {
    {"color_grid_basis.ktx2","color_grid.basis"}
};

vector<TextureSet> nonPo2TextureSets = {
    {"kodim17_basis.ktx2","kodim17.basis"}
};

vector<ktx_transcode_fmt_e> allFormats = {
    KTX_TTF_ETC1_RGB,
    KTX_TTF_ETC2_RGBA,
    KTX_TTF_BC1_RGB,
    KTX_TTF_BC3_RGBA,
    KTX_TTF_BC4_R,
    KTX_TTF_BC5_RG,
    KTX_TTF_BC7_M6_RGB,
    KTX_TTF_BC7_M5_RGBA,
    KTX_TTF_PVRTC1_4_RGB,
    KTX_TTF_PVRTC1_4_RGBA,
    KTX_TTF_ASTC_4x4_RGBA,
    KTX_TTF_PVRTC2_4_RGB,
    KTX_TTF_PVRTC2_4_RGBA,
    KTX_TTF_ETC2_EAC_R11,
    KTX_TTF_ETC2_EAC_RG11,
    KTX_TTF_RGBA32,
    KTX_TTF_RGB565,
    KTX_TTF_BGR565,
    KTX_TTF_RGBA4444
    // ATC and FXT1 formats are not supported by KTX2 as there
    // are no equivalent VkFormats.
};

vector<ktx_transcode_fmt_e> nonPo2Formats = {
    KTX_TTF_ETC1_RGB,
    KTX_TTF_ETC2_RGBA,
    KTX_TTF_BC1_RGB,
    KTX_TTF_BC3_RGBA,
    KTX_TTF_BC4_R,
    KTX_TTF_BC5_RG,
    KTX_TTF_BC7_M6_RGB,
    KTX_TTF_BC7_M5_RGBA,
    KTX_TTF_ASTC_4x4_RGBA,
    KTX_TTF_PVRTC2_4_RGB,
    KTX_TTF_PVRTC2_4_RGBA,
    KTX_TTF_ETC2_EAC_R11,
    KTX_TTF_ETC2_EAC_RG11,
    KTX_TTF_RGBA32,
    KTX_TTF_RGB565,
    KTX_TTF_BGR565,
    KTX_TTF_RGBA4444,

    // Don't support non power of two sizes (yet).
    KTX_TTF_PVRTC1_4_RGB,
    KTX_TTF_PVRTC1_4_RGBA

    // ATC and FXT1 formats are not supported by KTX2 as there
    // are no equivalent VkFormats.
};

class TextureCombinationsTest :
    public ::testing::TestWithParam<tuple<TextureSet,ktx_transcode_fmt_e>> {};

INSTANTIATE_TEST_CASE_P(AllCombinations,
                        TextureCombinationsTest,
                        ::testing::Combine(::testing::ValuesIn(allTextureSets),
                                           ::testing::ValuesIn(allFormats)));

INSTANTIATE_TEST_CASE_P(Po2Componations,
                        TextureCombinationsTest,
                        ::testing::Combine(::testing::ValuesIn(nonPo2TextureSets),
                                           ::testing::ValuesIn(nonPo2Formats)));

bool read_file( string path, void** data, long *fsize ) {
    FILE *f = fopen(path.data(),"rb");
    fseek(f, 0, SEEK_END);
    *fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

    *data = malloc(*fsize);
    fread(*data, 1, *fsize, f);
    fclose(f);
    return true;
}

void test_texture_set( TextureSet & textureSet, ktx_transcode_fmt_e format ) {

    void * basisData;
    long basisSize;
    
    string path = image_path+textureSet.basisuPath;
    bool read_success = read_file(path, &basisData, &basisSize);

    ASSERT_TRUE(read_success);

    basis_file basisu;

    basisu.open((uint8_t*)basisData,basisSize);
    uint32_t bWidth = basisu.getImageWidth(0,0);
    uint32_t bHeight = basisu.getImageHeight(0,0);

    uint32_t finalSize = basisu.getImageTranscodedSizeInBytes(0,0,format);
    ktx_uint8_t* basisTranscodedData = (ktx_uint8_t*) malloc(finalSize);
    basisu.startTranscoding();
    uint32_t bRes = basisu.transcodeImage((void*)basisTranscodedData,finalSize,0,0,format,0,0);

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
                                        format,
                                        0
                                        );
    ASSERT_EQ(result,KTX_SUCCESS) << "Format " << format;

    EXPECT_EQ(bWidth,newTex->baseWidth);
    EXPECT_EQ(bHeight,newTex->baseHeight);
    EXPECT_EQ(finalSize,newTex->dataSize);

    int cmp = std::memcmp(basisTranscodedData,newTex->pData,finalSize);

    ASSERT_EQ(cmp,0);

    free(data);
    free(basisTranscodedData);

    free(basisData);
}

TEST_P(TextureCombinationsTest, Basic) {
    TextureSet ts = get<0>(GetParam());
    ktx_transcode_fmt_e format = get<1>(GetParam());
    cout << "txt: " << ts.ktxPath
            << " format: " << format << "\n";
    test_texture_set(ts,format);
}
}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  image_path = string(argv[1]);
  return RUN_ALL_TESTS();
}
