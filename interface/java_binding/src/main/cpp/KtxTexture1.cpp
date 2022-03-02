/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <iostream>
#include "libktx-jni.h"

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture1_getGlFormat(JNIEnv *env,
                                                                                jobject thiz)
{
    return get_ktx1_texture(env, thiz)->glFormat;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture1_getGlInternalformat(JNIEnv *env,
                                                                                jobject thiz)
{
    return get_ktx1_texture(env, thiz)->glInternalformat;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture1_getGlBaseInternalformat(JNIEnv *env,
                                                                                jobject thiz)
{
    return get_ktx1_texture(env, thiz)->glBaseInternalformat;
}

extern "C" JNIEXPORT jobject JNICALL Java_org_khronos_ktx_KtxTexture1_create(JNIEnv *env,
                                                                                jobject,
                                                                                jobject java_create_info,
                                                                                jint storageAllocation)
{
    ktxTextureCreateInfo info;
    copy_ktx_texture_create_info(env, java_create_info, info);

    ktxTexture1 *instance = NULL;
    KTX_error_code result;

    ktxTextureCreateStorageEnum storage_alloc = static_cast<ktxTextureCreateStorageEnum>(storageAllocation);
    result = ktxTexture1_Create(&info, storage_alloc, &instance);

    assert (instance != NULL);

    if (result != KTX_SUCCESS)
    {
        std::cout << "Failure to create Ktx1Texture, error " << result << std::endl;
        return NULL;
    }

    return make_ktx1_wrapper(env, instance);
}

extern "C" JNIEXPORT jobject JNICALL Java_org_khronos_ktx_KtxTexture1_createFromNamedFile(JNIEnv *env,
                                                                                            jobject,
                                                                                            jstring filename,
                                                                                            jint createFlags)
{
    const char *filenameArray = env->GetStringUTFChars(filename, NULL);
    ktxTexture1 *instance = NULL;

    jint result = ktxTexture1_CreateFromNamedFile(filenameArray, createFlags, &instance);

    if (result != KTX_SUCCESS) {
        std::cout << "Failure to createFromNamedFile Ktx1Texture, error " << result << std::endl;
        return NULL;
    }

    assert (instance != NULL);

    return make_ktx1_wrapper(env, instance);
}

