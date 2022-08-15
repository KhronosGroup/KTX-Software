/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <iostream>
#include "libktx-jni.h"

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture2_getOETF(JNIEnv *env, jobject thiz)
{
    return ktxTexture2_GetOETF(get_ktx2_texture(env, thiz));
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_khronos_ktx_KtxTexture2_getPremultipliedAlpha(JNIEnv *env, jobject thiz)
{
    return ktxTexture2_GetPremultipliedAlpha(get_ktx2_texture(env, thiz));
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_khronos_ktx_KtxTexture2_needsTranscoding(JNIEnv *env, jobject thiz)
{
    return ktxTexture2_NeedsTranscoding(get_ktx2_texture(env, thiz));
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture2_getVkFormat(JNIEnv *env, jobject thiz)
{
    return get_ktx2_texture(env, thiz)->vkFormat;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture2_getSupercompressionScheme(JNIEnv *env, jobject thiz)
{
    return get_ktx2_texture(env, thiz)->supercompressionScheme;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture2_compressAstcEx(JNIEnv *env,
                                                                                jobject thiz,
                                                                                jobject jparams)
{
    ktxAstcParams params = {};
    copy_ktx_astc_params(env, jparams, params);

    return ktxTexture2_CompressAstcEx(get_ktx2_texture(env, thiz),
                                        &params);
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture2_compressAstc(JNIEnv *env,
                                                                                jobject thiz,
                                                                                jint quality)
{
    return ktxTexture2_CompressAstc(get_ktx2_texture(env, thiz),
                                        static_cast<uint32_t>(quality));
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture2_compressBasisEx(JNIEnv *env,
                                                                                jobject thiz,
                                                                                jobject jparams)
{
    ktxBasisParams params = {};
    copy_ktx_basis_params(env, jparams, params);

    return ktxTexture2_CompressBasisEx(get_ktx2_texture(env, thiz),
                                        &params);
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture2_compressBasis(JNIEnv *env,
                                                                                jobject thiz,
                                                                                jint quality)
{
    return ktxTexture2_CompressBasis(get_ktx2_texture(env, thiz), static_cast<uint32_t>(quality));
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture2_transcodeBasis(JNIEnv *env,
                                                                                jobject thiz,
                                                                                jint outputFormat,
                                                                                jint transcodeFlags)
{
    return ktxTexture2_TranscodeBasis(
        get_ktx2_texture(env, thiz),
        static_cast<ktx_transcode_fmt_e>(outputFormat),
        transcodeFlags);
}

extern "C" JNIEXPORT jobject JNICALL Java_org_khronos_ktx_KtxTexture2_create(JNIEnv *env,
             jobject,

             jobject jcreateInfo,

             jint jStorageAllocation)
{
    ktxTextureCreateInfo info;
    copy_ktx_texture_create_info(env, jcreateInfo, info);
    ktxTextureCreateStorageEnum storageAllocation = static_cast<ktxTextureCreateStorageEnum>(jStorageAllocation);

    ktxTexture2 *instance;
    KTX_error_code result;

    result = ktxTexture2_Create(&info, storageAllocation, &instance);

    if (result != KTX_SUCCESS)
    {
        std::cout << "Failure to create Ktx2Texture, error " << result << std::endl;
        return NULL;
    }

    assert (instance != NULL);

    jclass ktx_texture_class = env->FindClass("org/khronos/ktx/KtxTexture2");
    assert (ktx_texture_class != NULL);

    jmethodID ktx_texture_ctor = env->GetMethodID(ktx_texture_class, "<init>", "(J)V");
    jobject texture = env->NewObject(ktx_texture_class, ktx_texture_ctor, reinterpret_cast<jlong>(instance));

    return texture;
}

extern "C" JNIEXPORT jobject JNICALL Java_org_khronos_ktx_KtxTexture2_createFromNamedFile(JNIEnv *env,
                                                                                            jobject,
                                                                                            jstring filename,
                                                                                            jint createFlags)
{
    const char *filenameArray = env->GetStringUTFChars(filename, NULL);
    ktxTexture2 *instance = NULL;

    jint result = ktxTexture2_CreateFromNamedFile(filenameArray, createFlags, &instance);

    if (result != KTX_SUCCESS) {
        std::cout << "Failure to createFromNamedFile Ktx2Texture, error " << result << std::endl;
        return NULL;
    }

    assert (instance != NULL);

    return make_ktx2_wrapper(env, instance);
}


