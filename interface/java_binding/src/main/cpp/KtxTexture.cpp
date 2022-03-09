/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <cstring>
#include <jni.h>
#include <vector>
#include <iostream>

#include "ktx.h"
#include "libktx-jni.h"

struct pinned_image_buf
{
    jbyteArray handle;
    jbyte *data;
};

// The buffer list is a vector of memory buffers pinned by KTXTexture. These buffers are pinned
// when setImageFromMemory is used. They are freed when KTXTexture.destroy is invoked.

static std::vector<pinned_image_buf> *get_or_create_buffer_list(JNIEnv *env, jobject thiz)
{
    jclass ktx_texture_class = env->GetObjectClass(thiz);
    jfieldID ktx_buffers_field = env->GetFieldID(ktx_texture_class, "buffers", "J");

    std::vector<pinned_image_buf> *buffers =
        reinterpret_cast<std::vector<pinned_image_buf>*>(env->GetLongField(thiz, ktx_buffers_field));

    // Lazy init
    if (buffers == NULL) {
        buffers = new std::vector<pinned_image_buf>();
        env->SetLongField(thiz, ktx_buffers_field, reinterpret_cast<jlong>(buffers));
    }

    return buffers;
}

static void push_buffer_list(JNIEnv *env, jobject thiz, jbyteArray handle, jbyte *data)
{
    std::vector<pinned_image_buf> *buffers = get_or_create_buffer_list(env, thiz);

    pinned_image_buf buf;

    buf.handle = handle;
    buf.data = data;

    buffers->push_back(buf);
}

static void free_buffer_list(JNIEnv *env, jobject thiz)
{
    jclass ktx_texture_class = env->GetObjectClass(thiz);
    jfieldID ktx_buffers_field = env->GetFieldID(ktx_texture_class, "buffers", "J");

    std::vector<pinned_image_buf> *buffers =
        reinterpret_cast<std::vector<pinned_image_buf>*>(env->GetLongField(thiz, ktx_buffers_field));

    // Nothing to free
    if (buffers == NULL) {
        return;
    }

    std::vector<pinned_image_buf> l_buffers = *buffers;

    for (std::vector<pinned_image_buf>::iterator it = std::begin(l_buffers);
        it != std::end(l_buffers);
        ++it)
    {
        pinned_image_buf buffer = *it;

        env->ReleaseByteArrayElements(buffer.handle, buffer.data, JNI_ABORT);
    }

    env->SetLongField(thiz, ktx_buffers_field, 0);
    delete buffers;
}

extern "C" JNIEXPORT jsize JNICALL Java_org_khronos_ktx_KtxTexture_getBufferListSize(JNIEnv *env, jobject thiz)
{
    jclass ktx_texture_class = env->GetObjectClass(thiz);
    jfieldID ktx_buffers_field = env->GetFieldID(ktx_texture_class, "buffers", "J");

    std::vector<pinned_image_buf> *buffers =
        reinterpret_cast<std::vector<pinned_image_buf>*>(env->GetLongField(thiz, ktx_buffers_field));

    return static_cast<jsize>(buffers ? buffers->size() : 0);
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_khronos_ktx_KtxTexture_isArray(JNIEnv *env, jobject thiz)
{
    return get_ktx_texture(env, thiz)->isArray;
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_khronos_ktx_KtxTexture_isCubemap(JNIEnv *env, jobject thiz)
{
    return get_ktx_texture(env, thiz)->isCubemap;
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_khronos_ktx_KtxTexture_isCompressed(JNIEnv *env, jobject thiz)
{
    return get_ktx_texture(env, thiz)->isCompressed;
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_khronos_ktx_KtxTexture_getGenerateMipmaps(JNIEnv *env, jobject thiz)
{
    return get_ktx_texture(env, thiz)->generateMipmaps;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_getBaseWidth(JNIEnv *env, jobject thiz)
{
    return get_ktx_texture(env, thiz)->baseWidth;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_getBaseHeight(JNIEnv *env, jobject thiz)
{
    return get_ktx_texture(env, thiz)->baseHeight;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_getBaseDepth(JNIEnv *env, jobject thiz)
{
    return get_ktx_texture(env, thiz)->baseDepth;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_getNumDimensions(JNIEnv *env, jobject thiz)
{
    return get_ktx_texture(env, thiz)->numDimensions;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_getNumLevels(JNIEnv *env, jobject thiz)
{
    return get_ktx_texture(env, thiz)->numLevels;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_getNumLayers(JNIEnv *env, jobject thiz)
{
    return get_ktx_texture(env, thiz)->numLayers;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_getNumFaces(JNIEnv *env, jobject thiz)
{
    return get_ktx_texture(env, thiz)->numFaces;
}

extern "C" JNIEXPORT jbyteArray JNICALL Java_org_khronos_ktx_KtxTexture_getData(JNIEnv *env, jobject thiz)
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    ktx_uint8_t *data = ktxTexture_GetData(texture);

    if (data == NULL) {
        return NULL;
    }

    ktx_size_t dataSize = ktxTexture_GetDataSize(texture);

    jbyteArray outputArray = env->NewByteArray(static_cast<jsize>(dataSize));
    jsize outputLength = env->GetArrayLength(outputArray);

    if ((ktx_size_t) outputLength != dataSize) {
        return NULL;
    }

    env->SetByteArrayRegion(outputArray,
                            0,
                            static_cast<jsize>(dataSize),
                            reinterpret_cast<jbyte*>(data));

    return outputArray;
}

extern "C" JNIEXPORT jlong JNICALL Java_org_khronos_ktx_KtxTexture_getDataSize(JNIEnv *env, jobject thiz)
{
    return static_cast<jlong>(ktxTexture_GetDataSize(get_ktx_texture(env, thiz)));
}

extern "C" JNIEXPORT jlong JNICALL Java_org_khronos_KTXTexture_getDataSizeUncompressed(JNIEnv *env, jobject thiz)
{
    return ktxTexture_GetDataSizeUncompressed(get_ktx_texture(env, thiz));
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_getElementSize(JNIEnv *env, jobject thiz)
{
    return static_cast<jint>(ktxTexture_GetElementSize(get_ktx_texture(env, thiz)));
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_getRowPitch(JNIEnv *env,
                                                                            jobject thiz,
                                                                            jint level)
{
    return static_cast<jint>(ktxTexture_GetRowPitch(get_ktx_texture(env, thiz), level));
}

extern "C" JNIEXPORT jlong JNICALL Java_org_khronos_ktx_KtxTexture_getImageSize(JNIEnv *env,
                                                                            jobject thiz,
                                                                            jint level)
{
    return ktxTexture_GetImageSize(get_ktx_texture(env, thiz), level);
}

extern "C" JNIEXPORT jlong JNICALL Java_org_khronos_ktx_KtxTexture_getImageOffset(JNIEnv *env,
                                                                            jobject thiz,
                                                                            jint level,
                                                                            jint layer,
                                                                            jint faceSlice)
{
    ktx_size_t pOffset = 0;
    KTX_error_code result = ktxTexture_GetImageOffset(get_ktx_texture(env, thiz),
                                                        level,
                                                        layer,
                                                        faceSlice,
                                                        &pOffset);
    return static_cast<jlong>(result == KTX_SUCCESS
        ? pOffset
        : -1);
}

/* Useful methods :) (not properties) */

extern "C" JNIEXPORT void JNICALL Java_org_khronos_ktx_KtxTexture_destroy(JNIEnv *env, jobject thiz)
{
    ktxTexture_Destroy(get_ktx_texture(env, thiz));
    set_ktx_texture(env, thiz, NULL);
    free_buffer_list(env, thiz);
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_setImageFromMemory(JNIEnv *env,
                                                                                    jobject thiz,
                                                                                    jint level,
                                                                                    jint layer,
                                                                                    jint faceSlice,
                                                                                    jbyteArray srcArray)
{
    ktx_uint8_t *src = reinterpret_cast<ktx_uint8_t*>(env->GetByteArrayElements(srcArray, NULL));
    ktx_size_t srcSize = static_cast<ktx_size_t>(env->GetArrayLength(srcArray));

    jint result = ktxTexture_SetImageFromMemory(get_ktx_texture(env, thiz),
                                level,
                                layer,
                                faceSlice,
                                src,
                                srcSize);

    push_buffer_list(env, thiz, srcArray, reinterpret_cast<jbyte*>(src));
    /* DO NOT FREE SRC BUFFER NOW (see destroy()) */

    return result;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_writeToNamedFile(JNIEnv *env,
                                                                                    jobject thiz,
                                                                                    jstring dstName)
{
    const char *dstNameArray = env->GetStringUTFChars(dstName, NULL);

    jint result = ktxTexture_WriteToNamedFile(get_ktx_texture(env, thiz), dstNameArray);

    env->ReleaseStringUTFChars(dstName, dstNameArray);

    return result;
}

extern "C" JNIEXPORT jbyteArray JNICALL Java_org_khronos_ktx_KtxTexture_writeToMemory(JNIEnv *env,
                                                                                    jobject thiz)
{
    ktx_uint8_t *ppDstBytes;
    ktx_size_t pSize;
    KTX_error_code result = ktxTexture_WriteToMemory(get_ktx_texture(env, thiz), &ppDstBytes, &pSize);

    if (result != KTX_SUCCESS) {
        std::cout << "Failed to writeToMemory KTXTexture, error " << result << std::endl;
        return NULL;
    }

    jbyteArray out = env->NewByteArray(static_cast<jsize>(pSize));

    env->SetByteArrayRegion(out,
                            0,
                            static_cast<jsize>(pSize),
                            reinterpret_cast<const jbyte*>(ppDstBytes));

    delete ppDstBytes;

    return out;
}

