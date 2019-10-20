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


#ifdef DLL_EXPORT_FLAG
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

#include <ktx.h>
#include <ktx_sgd_helper.h>

extern "C" {

DLL_EXPORT ktxTexture2* aa_load_ktx( const uint8_t * data, size_t length, KTX_error_code* out_status ) {
    
    KTX_error_code result;
    
    ktxTexture2* newTex = 0;
    
    result = ktxTexture2_CreateFromMemory(
        (const ktx_uint8_t*) data,
        (ktx_size_t) length,
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        (ktxTexture2**)&newTex
        );

    *out_status = result;
    return newTex;
}

DLL_EXPORT class_id aa_ktx_get_classId ( ktxTexture2* ktx ) {
    return ktx->classId;
}
DLL_EXPORT ktx_bool_t aa_ktx_get_isArray ( ktxTexture2* ktx ) {
    return ktx->isArray;
}
DLL_EXPORT ktx_bool_t aa_ktx_get_isCubemap ( ktxTexture2* ktx ) {
    return ktx->isCubemap;
}
DLL_EXPORT ktx_bool_t aa_ktx_get_isCompressed ( ktxTexture2* ktx ) {
    return ktx->isCompressed;
}
DLL_EXPORT ktx_uint32_t aa_ktx_get_baseWidth ( ktxTexture2* ktx ) {
    return ktx->baseWidth;
}
DLL_EXPORT ktx_uint32_t aa_ktx_get_baseHeight ( ktxTexture2* ktx ) {
    return ktx->baseHeight;
}
DLL_EXPORT ktx_uint32_t aa_ktx_get_numDimensions ( ktxTexture2* ktx ) {
    return ktx->numDimensions;
}
DLL_EXPORT ktx_uint32_t aa_ktx_get_numLevels ( ktxTexture2* ktx ) {
    return ktx->numLevels;
}
DLL_EXPORT ktx_uint32_t aa_ktx_get_numLayers ( ktxTexture2* ktx ) {
    return ktx->numLayers;
}
DLL_EXPORT ktx_uint32_t aa_ktx_get_numFaces ( ktxTexture2* ktx ) {
    return ktx->numFaces;
}
DLL_EXPORT ktx_uint32_t aa_ktx_get_vkFormat ( ktxTexture2* ktx ) {
    return ktx->vkFormat;
}
DLL_EXPORT ktxSupercmpScheme aa_ktx_get_supercompressionScheme ( ktxTexture2* ktx ) {
    return ktx->supercompressionScheme;
}
DLL_EXPORT void aa_ktx_get_orientation (
    ktxTexture2* ktx,
    ktxOrientationX* x,
    ktxOrientationY* y,
    ktxOrientationZ* z
    )
{
    *x = ktx->orientation.x;
    *y = ktx->orientation.y;
    *z = ktx->orientation.z;
}

DLL_EXPORT bool aa_ktx_get_has_alpha( ktxTexture2* ktx ) {
    return ktxTexture2_getHasAlpha(ktx);
}

DLL_EXPORT KTX_error_code aa_transcode_ktx(
    ktxTexture2* ktx,
    ktx_transcode_fmt_e outputFormat,
    ktx_transcode_flags transcodeFlags
    )
{
    KTX_error_code result = ktxTexture2_TranscodeBasis(
       ktx,
       outputFormat,
       transcodeFlags
       );
    return result;
}

DLL_EXPORT void aa_ktx_get_data(
    ktxTexture2* ktx,
    const uint8_t ** data,
    size_t* length
    )
{
    *data = ktx->pData;
    *length = ktx->dataSize;
}

DLL_EXPORT void aa_unload_ktx( ktxTexture2* ktx ) {
    ktxTexture_Destroy((ktxTexture*)ktx);
}
}
