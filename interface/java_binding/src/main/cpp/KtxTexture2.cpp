/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <iostream>
#include "libktx-jni.h"

/**
 * Obtain the data of the given ByteBuffer.
 * 
 * The data may not be modified by callers!
 * 
 * This will take into account the position and limit of the given buffer,
 * and handle the case that the buffer is a direct- or an array-based 
 * buffer, and fill the given parameters accordingly:
 * 
 * The baseAddress will be the direct buffer address, or the start of
 * the array elements.
 * 
 * The actualAddress will be the base address, plus the 'position' of
 * the given buffer.
 * 
 * The length will be the 'limit-position' of the given buffer.
 * 
 * The data can only be assumed to be valid until releaseBufferData is
 * called, passing in the 'baseAddress' that was created by this function.
 * 
 * The returned data has to be considered to be read-only. Whether or
 * not changes will affect the given buffer depends on whether the
 * buffer is a direct- or an array-based buffer. The behavior when
 * modifying the returned data is not specified.
 * 
 * @param env The JNI environment pointer
 * @param buffer The buffer object
 * @param baseAddress The pointer that will receive the base address
 * @param actualAddress The pointer that will receive the actual address,
 * which is 'baseAddress + position'
 * @param length The pointer that will receive the length (which 
 * is 'limit-position')
 * @return Whether the buffer data could be obtained
 */
bool getBufferData(JNIEnv *env, jobject buffer, jbyte** baseAddress, jbyte **actualAddress, jint *length) {

  // Obtain the java.nio.Buffer class and its required methods
  jclass bufferClass = env->FindClass("java/nio/Buffer");
  jmethodID positionMethod = env->GetMethodID(bufferClass, "position", "()I");
  jmethodID limitMethod = env->GetMethodID(bufferClass, "limit", "()I");
  jmethodID isDirectMethod = env->GetMethodID(bufferClass, "isDirect", "()Z");
  jmethodID hasArrayMethod = env->GetMethodID(bufferClass, "hasArray", "()Z");
  jmethodID arrayMethod = env->GetMethodID(bufferClass, "array", "()Ljava/lang/Object;");

  // Obtain the position and limit of the buffer
  jint position = env->CallIntMethod(buffer, positionMethod);
  jint limit = env->CallIntMethod(buffer, limitMethod);

  // If the buffer is direct, then compute the results from
  // the direct buffer address
  jboolean isDirect = env->CallBooleanMethod(buffer, isDirectMethod);
  if (isDirect == JNI_TRUE) {
    jbyte* start = static_cast<jbyte*>(env->GetDirectBufferAddress(buffer));
    *baseAddress = start;
    *actualAddress = start + position;
    *length = (position - limit);
    return true;
  }

  // If the buffer is backed by an array, then compute the
  // results from the array elements. (These have to be 
  // released in the releaseBufferData call!)
  jboolean hasArray = env->CallBooleanMethod(buffer, hasArrayMethod);
  if (hasArray == JNI_TRUE) {
    jobject array = env->CallObjectMethod(buffer, arrayMethod);
    jbyte* start = env->GetByteArrayElements(static_cast<jbyteArray>(array), NULL);
    *baseAddress = start;
    *actualAddress = start + position;
    *length = (position - limit);
    return true;
  }

  return false;
}

/**
 * Release the buffer data that was obtained with 'getBufferData'.
 * 
 * The given baseAddress has to be the address that was obtained from
 * the 'getBufferData' call. For the case that the buffer is an 
 * array-backed buffer, this function will NOT write back any
 * changes that may have been made in the buffer data.
 * 
 * @param env The JNI environment pointer
 * @param buffer The buffer object
 * @param baseAddress The base address obtained with 'getBufferData'
 */
void releaseBufferData(JNIEnv *env, jobject buffer, jbyte* baseAddress) {

  // Obtain the java.nio.Buffer class and its required methods
  jclass bufferClass = env->FindClass("java/nio/Buffer");
  jmethodID isDirectMethod = env->GetMethodID(bufferClass, "isDirect", "()Z");
  jmethodID hasArrayMethod = env->GetMethodID(bufferClass, "hasArray", "()Z");
  jmethodID arrayMethod = env->GetMethodID(bufferClass, "array", "()Ljava/lang/Object;");

  // For direct buffers, nothing has to be done
  jboolean isDirect = env->CallBooleanMethod(buffer, isDirectMethod);
  if (isDirect == JNI_FALSE) {
    return;
  }

  // For array-backed buffers, the elements are released, without
  // writing back any changes
  jboolean hasArray = env->CallBooleanMethod(buffer, hasArrayMethod);
  if (hasArray == JNI_TRUE) {
    jobject array = env->CallObjectMethod(buffer, arrayMethod);
    env->ReleaseByteArrayElements(static_cast<jbyteArray>(array), baseAddress, JNI_ABORT);
  }
}


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
    copy_ktx_astc_params(env, jparams, params);

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
    copy_ktx_basis_params(env, jparams, params);

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
    if (filename == NULL) 
    {
      ThrowByName(env, "java/lang/NullPointerException", "Parameter 'filename' is null for createFromNamedFile");
      return NULL;
    }

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
        std::cout << "Could not obtain data from input buffer" << std::endl;
        return NULL;
    }
    ktxTexture2 *instance = NULL;
    ktx_uint8_t *inputData = reinterpret_cast<ktx_uint8_t*>(actualAddress);
    ktx_size_t size = static_cast<ktx_size_t>(length);

    jint result = ktxTexture2_CreateFromMemory(inputData, size, createFlags, &instance);
    releaseBufferData(env, byteBuffer, baseAddress);
    if (result != KTX_SUCCESS) {
        std::cout << "Failure to createFromMemory Ktx2Texture, error " << result << std::endl;
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
