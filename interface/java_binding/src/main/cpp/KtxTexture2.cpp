/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * Copyright (c) 2024, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <iostream>
#include "libktx-jni.h"

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture2_getOETF(JNIEnv *env, jobject thiz)
{
    ktxTexture2 *texture = get_ktx2_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return ktxTexture2_GetOETF(texture);
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_khronos_ktx_KtxTexture2_getPremultipliedAlpha(JNIEnv *env, jobject thiz)
{
    ktxTexture2 *texture = get_ktx2_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return ktxTexture2_GetPremultipliedAlpha(texture);
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_khronos_ktx_KtxTexture2_needsTranscoding(JNIEnv *env, jobject thiz)
{
    ktxTexture2 *texture = get_ktx2_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return false;
    }
    return ktxTexture2_NeedsTranscoding(texture);
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture2_getVkFormat(JNIEnv *env, jobject thiz)
{
    ktxTexture2 *texture = get_ktx2_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return texture->vkFormat;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture2_getSupercompressionScheme(JNIEnv *env, jobject thiz)
{
    ktxTexture2 *texture = get_ktx2_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return texture->supercompressionScheme;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture2_compressAstcEx(JNIEnv *env,
                                                                                jobject thiz,
                                                                                jobject jparams)
{
    if (jparams == NULL) 
    {
      ThrowByName(env, "java/lang/NullPointerException", "Parameter 'jparams' is null for compressAstcEx");
      return 0;
    }
    ktxTexture2 *texture = get_ktx2_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }

    ktxAstcParams params = {};
    if (!copy_ktx_astc_params(env, jparams, params)) 
    {
        // Exception is already pending
        return 0;
    }

    return ktxTexture2_CompressAstcEx(texture, &params);
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture2_compressAstc(JNIEnv *env,
                                                                                jobject thiz,
                                                                                jint quality)
{
    ktxTexture2 *texture = get_ktx2_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return ktxTexture2_CompressAstc(texture, static_cast<uint32_t>(quality));
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture2_compressBasisEx(JNIEnv *env,
                                                                                jobject thiz,
                                                                                jobject jparams)
{
    if (jparams == NULL) 
    {
      ThrowByName(env, "java/lang/NullPointerException", "Parameter 'jparams' is null for compressBasisEx");
      return 0;
    }
    ktxTexture2 *texture = get_ktx2_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }

    ktxBasisParams params = {};
    if (!copy_ktx_basis_params(env, jparams, params)) 
    {
        // Exception is already pending
        return 0;
    }

    return ktxTexture2_CompressBasisEx(texture, &params);
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture2_compressBasis(JNIEnv *env,
                                                                                jobject thiz,
                                                                                jint quality)
{
    ktxTexture2 *texture = get_ktx2_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return ktxTexture2_CompressBasis(texture, static_cast<uint32_t>(quality));
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture2_transcodeBasis(JNIEnv *env,
                                                                                jobject thiz,
                                                                                jint outputFormat,
                                                                                jint transcodeFlags)
{
    ktxTexture2 *texture = get_ktx2_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return ktxTexture2_TranscodeBasis(
        texture,
        static_cast<ktx_transcode_fmt_e>(outputFormat),
        transcodeFlags);
}

extern "C" JNIEXPORT jobject JNICALL Java_org_khronos_ktx_KtxTexture2_create(JNIEnv *env,
             jobject,
             jobject jcreateInfo,
             jint jStorageAllocation)
{
    if (jcreateInfo == NULL) 
    {
      ThrowByName(env, "java/lang/NullPointerException", "Parameter 'jcreateInfo' is null for create");
      return NULL;
    }

    ktxTextureCreateInfo info;
    copy_ktx_texture_create_info(env, jcreateInfo, info);
    ktxTextureCreateStorageEnum storageAllocation = static_cast<ktxTextureCreateStorageEnum>(jStorageAllocation);

    ktxTexture2 *instance;
    KTX_error_code result = ktxTexture2_Create(&info, storageAllocation, &instance);

    if (result != KTX_SUCCESS)
    {
        ThrowByName(env, "org/khronos/ktx/KtxException", ktxErrorString(result));
        return NULL;
    }

    assert (instance != NULL);

    return make_ktx2_wrapper(env, instance);
}

extern "C" JNIEXPORT jobject JNICALL Java_org_khronos_ktx_KtxTexture2_createFromNamedFile(JNIEnv *env,
                                                                                            jobject,
                                                                                            jstring filename,
                                                                                            jint createFlags)
{
    if (filename == NULL) 
    {
      ThrowByName(env, "java/lang/NullPointerException", "Parameter 'filename' is null for createFromNamedFile");
      return NULL;
    }

    const char *filenameArray = env->GetStringUTFChars(filename, NULL);
    if (filenameArray == NULL) 
    {
      // OutOfMemoryError is already pending
      return NULL;
    }
    ktxTexture2 *instance = NULL;

    KTX_error_code result = ktxTexture2_CreateFromNamedFile(filenameArray, createFlags, &instance);

    if (result != KTX_SUCCESS) {
        ThrowByName(env, "org/khronos/ktx/KtxException", ktxErrorString(result));
        return NULL;
    }

    assert (instance != NULL);

    return make_ktx2_wrapper(env, instance);
}

extern "C" JNIEXPORT jobject JNICALL Java_org_khronos_ktx_KtxTexture2_createFromMemory(JNIEnv *env,
                                                                                       jclass, 
                                                                                       jobject byteBuffer,
                                                                                       jint createFlags)
{
    if (byteBuffer == NULL) 
    {
      ThrowByName(env, "java/lang/NullPointerException", "Parameter 'byteBuffer' is null for createFromMemory");
      return NULL;
    }

    jbyte *baseAddress = NULL;
    jbyte *actualAddress = NULL;
    jint length = 0;
    bool acquired = getBufferData(env, byteBuffer, &baseAddress, &actualAddress, &length);
    if (!acquired) {
        ThrowByName(env, "org/khronos/ktx/KtxException", "Could not obtain data from input buffer");
        return NULL;
    }
    ktxTexture2 *instance = NULL;
    ktx_uint8_t *inputData = reinterpret_cast<ktx_uint8_t*>(actualAddress);
    ktx_size_t size = static_cast<ktx_size_t>(length);

    KTX_error_code result = ktxTexture2_CreateFromMemory(inputData, size, createFlags, &instance);
    releaseBufferData(env, byteBuffer, baseAddress);
    if (env->ExceptionCheck()) {
      return NULL;
    }
    if (result != KTX_SUCCESS) {
        ThrowByName(env, "org/khronos/ktx/KtxException", ktxErrorString(result));
        return NULL;
    }
    assert (instance != NULL);
    return make_ktx2_wrapper(env, instance);
}


extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture2_deflateZstd(JNIEnv *env,
                                                                               jobject thiz,
                                                                               jint level)
{
    ktxTexture2 *texture = get_ktx2_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return ktxTexture2_DeflateZstd(texture, static_cast<ktx_uint32_t>(level));
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture2_deflateZLIB(JNIEnv *env,
                                                                               jobject thiz,
                                                                               jint level)
{
    ktxTexture2 *texture = get_ktx2_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return ktxTexture2_DeflateZLIB(texture, static_cast<ktx_uint32_t>(level));
}
