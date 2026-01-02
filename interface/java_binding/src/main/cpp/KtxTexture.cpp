/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * Copyright (c) 2024, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <cstring>
#include <cstdint>
#include <jni.h>
#include <vector>
#include <iostream>

#include "ktx.h"
#include "libktx-jni.h"

extern "C" JNIEXPORT jboolean JNICALL Java_org_khronos_ktx_KtxTexture_isArray(JNIEnv *env, jobject thiz)
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return false;
    }
    return texture->isArray;
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_khronos_ktx_KtxTexture_isCubemap(JNIEnv *env, jobject thiz)
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return false;
    }
    return texture->isCubemap;
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_khronos_ktx_KtxTexture_isCompressed(JNIEnv *env, jobject thiz)
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return false;
    }
    return texture->isCompressed;
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_khronos_ktx_KtxTexture_getGenerateMipmaps(JNIEnv *env, jobject thiz)
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return false;
    }
    return texture->generateMipmaps;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_getBaseWidth(JNIEnv *env, jobject thiz)
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return texture->baseWidth;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_getBaseHeight(JNIEnv *env, jobject thiz)
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return texture->baseHeight;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_getBaseDepth(JNIEnv *env, jobject thiz)
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return texture->baseDepth;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_getNumDimensions(JNIEnv *env, jobject thiz)
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return texture->numDimensions;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_getNumLevels(JNIEnv *env, jobject thiz)
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return texture->numLevels;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_getNumLayers(JNIEnv *env, jobject thiz)
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return texture->numLayers;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_getNumFaces(JNIEnv *env, jobject thiz)
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return texture->numFaces;
}

extern "C" JNIEXPORT jbyteArray JNICALL Java_org_khronos_ktx_KtxTexture_getData(JNIEnv *env, jobject thiz)
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return NULL;
    }
    ktx_uint8_t *data = ktxTexture_GetData(texture);

    if (data == NULL) {
        return NULL;
    }

    ktx_size_t dataSize = ktxTexture_GetDataSize(texture);

    if (dataSize >= UINT32_MAX) {
        ThrowByName(env, "java/lang/UnsupportedOperationException", "The array returned by getData is too large for a Java array");
        return NULL;
    }

    jbyteArray outputArray = env->NewByteArray(static_cast<jsize>(dataSize));
    if (outputArray == NULL) 
    {
      // OutOfMemoryError is already pending
      return NULL;
    }

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
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return static_cast<jlong>(ktxTexture_GetDataSize(texture));
}

extern "C" JNIEXPORT jlong JNICALL Java_org_khronos_ktx_KtxTexture_getDataSizeUncompressed(JNIEnv *env, jobject thiz)
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return ktxTexture_GetDataSizeUncompressed(texture);
}


extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_glUpload(JNIEnv *env, jobject thiz, jintArray javaTexture, jintArray javaTarget, jintArray javaGlError) 
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }

    // The target array may not be NULL, and must have
    // a size of at least 1
    if (javaTarget == NULL) 
    {
      ThrowByName(env, "java/lang/NullPointerException", "Parameter 'target' is null for glUpload");
      return 0;
    }
    jsize javaTargetSize = env->GetArrayLength(javaTarget);
    if (javaTargetSize == 0) 
    {
      ThrowByName(env, "java/lang/IllegalArgumentException", "Parameter 'target' may not have length 0");
      return 0;
    }

    // The texture array may be NULL, but if it is not NULL,
    // then it must have a length of at least 1
    if (javaTexture != NULL) 
    {
      jsize javaTextureSize = env->GetArrayLength(javaTexture);
      if (javaTextureSize == 0) 
      {
        ThrowByName(env, "java/lang/IllegalArgumentException", "Parameter 'texture' may not have length 0");
        return 0;
      }
    }

    // The GL error array may be NULL, but if it is not NULL,
    // then it must have a length of at least 1
    if (javaGlError != NULL) 
    {
      jsize javaGlErrorSize = env->GetArrayLength(javaGlError);
      if (javaGlErrorSize == 0) 
      {
        ThrowByName(env, "java/lang/IllegalArgumentException", "Parameter 'glError' may not have length 0");
        return 0;
      }
    }

    GLuint textureValue = 0;
    if (javaTexture != NULL) 
    {
      jint *javaTextureArrayElements = env->GetIntArrayElements(javaTexture, NULL);
      if (javaTextureArrayElements == NULL) 
      {
        // OutOfMemoryError is already pending
        return 0;
      }
      textureValue = static_cast<GLuint>(javaTextureArrayElements[0]);
      env->ReleaseIntArrayElements(javaTexture, javaTextureArrayElements, JNI_ABORT);
    }

    GLenum target; 
    GLenum glError;

    KTX_error_code result = ktxTexture_GLUpload(texture, &textureValue, &target, &glError);

    // Write back the texture into the array
    if (javaTexture != NULL) 
    {
      jint *javaTextureArrayElements = env->GetIntArrayElements(javaTexture, NULL);
      if (javaTextureArrayElements == NULL) 
      {
        // OutOfMemoryError is already pending
        return 0;
      }
      javaTextureArrayElements[0] = static_cast<jint>(textureValue);
      env->ReleaseIntArrayElements(javaTexture, javaTextureArrayElements, JNI_COMMIT);
    }

    // Write back the target into the array
    if (javaTarget != NULL) 
    {
      jint *javaTargetArrayElements = env->GetIntArrayElements(javaTarget, NULL);
      if (javaTargetArrayElements == NULL) 
      {
        // OutOfMemoryError is already pending
        return 0;
      }
      javaTargetArrayElements[0] = static_cast<jint>(target);
      env->ReleaseIntArrayElements(javaTarget, javaTargetArrayElements, JNI_COMMIT);
    }

    // Write back the error into the array
    if (javaGlError != NULL) 
    {
      jint *javaGlErrorArrayElements = env->GetIntArrayElements(javaGlError, NULL);
      if (javaGlErrorArrayElements == NULL) 
      {
        // OutOfMemoryError is already pending
        return 0;
      }
      javaGlErrorArrayElements[0] = static_cast<jint>(glError);
      env->ReleaseIntArrayElements(javaGlError, javaGlErrorArrayElements, JNI_COMMIT);
    }
    return result;
}




extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_getElementSize(JNIEnv *env, jobject thiz)
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return static_cast<jint>(ktxTexture_GetElementSize(texture));
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_getRowPitch(JNIEnv *env,
                                                                            jobject thiz,
                                                                            jint level)
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return static_cast<jint>(ktxTexture_GetRowPitch(texture, level));
}

extern "C" JNIEXPORT jlong JNICALL Java_org_khronos_ktx_KtxTexture_getImageSize(JNIEnv *env,
                                                                            jobject thiz,
                                                                            jint level)
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }
    return ktxTexture_GetImageSize(texture, level);
}

extern "C" JNIEXPORT jlong JNICALL Java_org_khronos_ktx_KtxTexture_getImageOffset(JNIEnv *env,
                                                                            jobject thiz,
                                                                            jint level,
                                                                            jint layer,
                                                                            jint faceSlice)
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }

    ktx_size_t pOffset = 0;
    KTX_error_code result = ktxTexture_GetImageOffset(texture,
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
    // Calling 'destroy' on an already destroyed texture 
    // should be OK, but have no effect
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture != NULL) 
    {
        ktxTexture_Destroy(get_ktx_texture(env, thiz));
        set_ktx_texture(env, thiz, NULL);
    }
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_setImageFromMemory(JNIEnv *env,
                                                                                    jobject thiz,
                                                                                    jint level,
                                                                                    jint layer,
                                                                                    jint faceSlice,
                                                                                    jbyteArray srcArray)
{
    if (srcArray == NULL) 
    {
      ThrowByName(env, "java/lang/NullPointerException", "Parameter 'srcArray' is null for setImageFromMemory");
      return 0;
    }
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }

    jbyte* srcArrayElements = env->GetByteArrayElements(srcArray, NULL);
    if (srcArrayElements == NULL) 
    {
      // OutOfMemoryError is already pending
      return 0;
    }

    ktx_uint8_t *src = reinterpret_cast<ktx_uint8_t*>(srcArrayElements);
    ktx_size_t srcSize = static_cast<ktx_size_t>(env->GetArrayLength(srcArray));

    jint result = ktxTexture_SetImageFromMemory(texture,
                                level,
                                layer,
                                faceSlice,
                                src,
                                srcSize);

    env->ReleaseByteArrayElements(srcArray, reinterpret_cast<jbyte*>(src), JNI_ABORT);

    return result;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KtxTexture_writeToNamedFile(JNIEnv *env,
                                                                                    jobject thiz,
                                                                                    jstring dstName)
{
    if (dstName == NULL) 
    {
      ThrowByName(env, "java/lang/NullPointerException", "Parameter 'dstName' is null for writeToNamedFile");
      return 0;
    }
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }

    const char *dstNameArray = env->GetStringUTFChars(dstName, NULL);
    if (dstNameArray == NULL) 
    {
      // OutOfMemoryError is already pending
      return 0;
    }


    jint result = ktxTexture_WriteToNamedFile(texture, dstNameArray);

    env->ReleaseStringUTFChars(dstName, dstNameArray);

    return result;
}

extern "C" JNIEXPORT jbyteArray JNICALL Java_org_khronos_ktx_KtxTexture_writeToMemory(JNIEnv *env,
                                                                                    jobject thiz)
{
    ktxTexture *texture = get_ktx_texture(env, thiz);
    if (texture == NULL) 
    {
      ThrowDestroyed(env);
      return 0;
    }

    ktx_uint8_t *ppDstBytes;
    ktx_size_t pSize;
    KTX_error_code result = ktxTexture_WriteToMemory(texture, &ppDstBytes, &pSize);

    if (result != KTX_SUCCESS) {
        ThrowByName(env, "org/khronos/ktx/KtxException", ktxErrorString(result));
        return NULL;
    }
    if (pSize >= UINT32_MAX) {
        delete ppDstBytes;// make sure to delete it
        ThrowByName(env, "java/lang/UnsupportedOperationException", "The array created by by writeToMemory is too large for a Java array");
        return NULL;
    }

    jbyteArray out = env->NewByteArray(static_cast<jsize>(pSize));
    if (out == NULL) 
    {
      // OutOfMemoryError is already pending
      return NULL;
    }

    env->SetByteArrayRegion(out,
                            0,
                            static_cast<jsize>(pSize),
                            reinterpret_cast<const jbyte*>(ppDstBytes));

    delete ppDstBytes;

    return out;
}

