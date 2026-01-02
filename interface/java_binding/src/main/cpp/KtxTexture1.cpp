/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * Copyright (c) 2024, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <iostream>
#include "libktx-jni.h"

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture1_getGlFormat(JNIEnv *env,
                                                                                jobject thiz)
{
    ktxTexture1 *texture = get_ktx1_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return texture->glFormat;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture1_getGlInternalformat(JNIEnv *env,
                                                                                jobject thiz)
{
    ktxTexture1 *texture = get_ktx1_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return texture->glInternalformat;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture1_getGlBaseInternalformat(JNIEnv *env,
                                                                                jobject thiz)
{
    ktxTexture1 *texture = get_ktx1_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return texture->glBaseInternalformat;
}

extern "C" JNIEXPORT jobject JNICALL Java_org_khronos_ktx_KtxTexture1_create(JNIEnv *env,
                                                                                jobject,
                                                                                jobject java_create_info,
                                                                                jint storageAllocation)
{
    if (java_create_info == NULL) 
    {
      ThrowByName(env, "java/lang/NullPointerException", "Parameter 'java_create_info' is null for create");
      return NULL;
    }

    ktxTextureCreateInfo info;
    copy_ktx_texture_create_info(env, java_create_info, info);

    ktxTexture1 *instance = NULL;

    ktxTextureCreateStorageEnum storage_alloc = static_cast<ktxTextureCreateStorageEnum>(storageAllocation);
    KTX_error_code result = ktxTexture1_Create(&info, storage_alloc, &instance);

    assert (instance != NULL);

    if (result != KTX_SUCCESS)
    {
        ThrowByName(env, "org/khronos/ktx/KtxException", ktxErrorString(result));
        return NULL;
    }

    return make_ktx1_wrapper(env, instance);
}

extern "C" JNIEXPORT jobject JNICALL Java_org_khronos_ktx_KtxTexture1_createFromNamedFile(JNIEnv *env,
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

    ktxTexture1 *instance = NULL;

    ktx_error_code_e result = ktxTexture1_CreateFromNamedFile(filenameArray, createFlags, &instance);

    if (result != KTX_SUCCESS) {
        ThrowByName(env, "org/khronos/ktx/KtxException", ktxErrorString(result));
        return NULL;
    }

    assert (instance != NULL);

    return make_ktx1_wrapper(env, instance);
}

