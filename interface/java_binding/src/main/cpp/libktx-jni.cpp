/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * Copyright (c) 2024, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include "libktx-jni.h"

#include <iostream>

// The global references to the required Java classes
jclass KtxTexture1_class;
jclass KtxTexture2_class;
jclass Buffer_class;

// The method IDs for the constructors of the KTX Java classes
jmethodID KtxTexture1_constructor;
jmethodID KtxTexture2_constructor;

// The method IDs for the Java classes
// The comment indicates their signature/type
jmethodID Buffer_position_method; // "()I"
jmethodID Buffer_limit_method; // "()I"
jmethodID Buffer_isDirect_method; // "()Z"
jmethodID Buffer_hasArray_method; // "()Z"
jmethodID Buffer_array_method; // "()Ljava/lang/Object;"

// The field IDs of the Java classes 
// The comment indicates their signature/type

jfieldID KtxTexture_instance_field; // "J"

jfieldID KtxTextureCreateInfo_glInternalformat_field; // "I"
jfieldID KtxTextureCreateInfo_baseWidth_field; ; // "I"
jfieldID KtxTextureCreateInfo_baseHeight_field; // "I"
jfieldID KtxTextureCreateInfo_baseDepth_field; // "I"
jfieldID KtxTextureCreateInfo_numDimensions_field; // "I"
jfieldID KtxTextureCreateInfo_numLevels_field; // "I"
jfieldID KtxTextureCreateInfo_numLayers_field; // "I"
jfieldID KtxTextureCreateInfo_numFaces_field; // "I"
jfieldID KtxTextureCreateInfo_isArray_field; // "Z"
jfieldID KtxTextureCreateInfo_generateMipmaps_field; // "Z"
jfieldID KtxTextureCreateInfo_vkFormat_field; // "I"

jfieldID KtxAstcParams_verbose_field; // "Z"
jfieldID KtxAstcParams_threadCount_field; // "I"
jfieldID KtxAstcParams_blockDimension_field; // "I"
jfieldID KtxAstcParams_mode_field; // "I"
jfieldID KtxAstcParams_qualityLevel_field; // "I"
jfieldID KtxAstcParams_normalMap_field; // "Z"
jfieldID KtxAstcParams_perceptual_field; // "Z"
jfieldID KtxAstcParams_inputSwizzle_field; // "[C"

jfieldID KtxBasisParams_uastc_field; // "Z"
jfieldID KtxBasisParams_verbose_field; // "Z"
jfieldID KtxBasisParams_noSSE_field; // "Z"
jfieldID KtxBasisParams_threadCount_field; // "I"
jfieldID KtxBasisParams_compressionLevel_field; // "I"
jfieldID KtxBasisParams_qualityLevel_field; // "I"
jfieldID KtxBasisParams_maxEndpoints_field; // "I"
jfieldID KtxBasisParams_endpointRDOThreshold_field; // "F"
jfieldID KtxBasisParams_maxSelectors_field; // "I"
jfieldID KtxBasisParams_selectorRDOThreshold_field; // "F"
jfieldID KtxBasisParams_inputSwizzle_field; // "[C"
jfieldID KtxBasisParams_normalMap_field; // "Z"
jfieldID KtxBasisParams_preSwizzle_field; // "Z"
jfieldID KtxBasisParams_noEndpointRDO_field; // "Z"
jfieldID KtxBasisParams_noSelectorRDO_field; // "Z"
jfieldID KtxBasisParams_uastcFlags_field; // "I"
jfieldID KtxBasisParams_uastcRDO_field; // "Z"
jfieldID KtxBasisParams_uastcRDOQualityScalar_field; // "F"
jfieldID KtxBasisParams_uastcRDODictSize_field; // "I"
jfieldID KtxBasisParams_uastcRDOMaxSmoothBlockErrorScale_field; // "F"
jfieldID KtxBasisParams_uastcRDOMaxSmoothBlockStdDev_field; // "F"
jfieldID KtxBasisParams_uastcRDODontFavorSimplerModes_field; // "Z"
jfieldID KtxBasisParams_uastcRDONoMultithreading_field; // "Z"


/**
 * Called when the library is loaded. 
 * 
 * This will initialize all required classes, field IDs, and method IDs
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *)
{
    JNIEnv *env = NULL;
    if (jvm->GetEnv((void **)&env, JNI_VERSION_1_4))
    {
        return JNI_ERR;
    }

    jclass cls = NULL;

    // Obtain the KtxTexture1 class and its constructor
    if (!initClass(env, cls, "org/khronos/ktx/KtxTexture1")) return JNI_ERR;
    if (!initMethod(env, cls, KtxTexture1_constructor, "<init>", "(J)V")) return JNI_ERR;
    KtxTexture1_class = (jclass)env->NewGlobalRef(cls);
    if (KtxTexture1_class == NULL)
    {
        std::cerr << "Failed to create reference to KtxTexture1 class " << std::endl;
        return JNI_ERR;
    }

    // Obtain the KtxTexture2 class and its constructor
    if (!initClass(env, cls, "org/khronos/ktx/KtxTexture2")) return JNI_ERR;
    if (!initMethod(env, cls, KtxTexture2_constructor, "<init>", "(J)V")) return JNI_ERR;
    KtxTexture2_class = (jclass)env->NewGlobalRef(cls);
    if (KtxTexture1_class == NULL)
    {
        std::cerr << "Failed to create reference to KtxTexture2 class " << std::endl;
        return JNI_ERR;
    }

    // Obtain the method IDs for the Buffer class
    if (!initClass(env, cls, "java/nio/Buffer")) return JNI_ERR;
    if (!initMethod(env, cls, Buffer_position_method, "position", "()I")) return JNI_ERR;
    if (!initMethod(env, cls, Buffer_limit_method, "limit", "()I")) return JNI_ERR;
    if (!initMethod(env, cls, Buffer_isDirect_method, "isDirect", "()Z")) return JNI_ERR;
    if (!initMethod(env, cls, Buffer_hasArray_method, "hasArray", "()Z")) return JNI_ERR;
    if (!initMethod(env, cls, Buffer_array_method, "array", "()Ljava/lang/Object;")) return JNI_ERR;

    // Obtain the fieldIDs of the KtxTexture class
    if (!initClass(env, cls, "org/khronos/ktx/KtxTexture")) return JNI_ERR;
    if (!initField(env, cls, KtxTexture_instance_field, "instance", "J")) return JNI_ERR;

    // Obtain the fieldIDs of the KtxTextureCreateInfo class
    if (!initClass(env, cls, "org/khronos/ktx/KtxTextureCreateInfo")) return JNI_ERR;
    if (!initField(env, cls, KtxTextureCreateInfo_glInternalformat_field, "glInternalformat", "I")) return JNI_ERR;
    if (!initField(env, cls, KtxTextureCreateInfo_baseWidth_field, "baseWidth", "I")) return JNI_ERR;
    if (!initField(env, cls, KtxTextureCreateInfo_baseHeight_field, "baseHeight", "I")) return JNI_ERR;
    if (!initField(env, cls, KtxTextureCreateInfo_baseDepth_field, "baseDepth", "I")) return JNI_ERR;
    if (!initField(env, cls, KtxTextureCreateInfo_numDimensions_field, "numDimensions", "I")) return JNI_ERR;
    if (!initField(env, cls, KtxTextureCreateInfo_numLevels_field, "numLevels", "I")) return JNI_ERR;
    if (!initField(env, cls, KtxTextureCreateInfo_numLayers_field, "numLayers", "I")) return JNI_ERR;
    if (!initField(env, cls, KtxTextureCreateInfo_numFaces_field, "numFaces", "I")) return JNI_ERR;
    if (!initField(env, cls, KtxTextureCreateInfo_isArray_field, "isArray", "Z")) return JNI_ERR;
    if (!initField(env, cls, KtxTextureCreateInfo_generateMipmaps_field, "generateMipmaps", "Z")) return JNI_ERR;
    if (!initField(env, cls, KtxTextureCreateInfo_vkFormat_field, "vkFormat", "I")) return JNI_ERR;

    // Obtain the fieldIDs of the KtxAstcParams class
    if (!initClass(env, cls, "org/khronos/ktx/KtxAstcParams")) return JNI_ERR;
    if (!initField(env, cls, KtxAstcParams_verbose_field, "verbose", "Z")) return JNI_ERR;
    if (!initField(env, cls, KtxAstcParams_threadCount_field, "threadCount", "I")) return JNI_ERR;
    if (!initField(env, cls, KtxAstcParams_blockDimension_field, "blockDimension", "I")) return JNI_ERR;
    if (!initField(env, cls, KtxAstcParams_mode_field, "mode", "I")) return JNI_ERR;
    if (!initField(env, cls, KtxAstcParams_qualityLevel_field, "qualityLevel", "I")) return JNI_ERR;
    if (!initField(env, cls, KtxAstcParams_normalMap_field, "normalMap", "Z")) return JNI_ERR;
    if (!initField(env, cls, KtxAstcParams_perceptual_field, "perceptual", "Z")) return JNI_ERR;
    if (!initField(env, cls, KtxAstcParams_inputSwizzle_field, "inputSwizzle", "[C")) return JNI_ERR;

    // Obtain the fieldIDs of the KtxBasisParams class
    if (!initClass(env, cls, "org/khronos/ktx/KtxBasisParams")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_uastc_field, "uastc", "Z")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_verbose_field, "verbose", "Z")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_noSSE_field, "noSSE", "Z")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_threadCount_field, "threadCount", "I")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_compressionLevel_field, "compressionLevel", "I")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_qualityLevel_field, "qualityLevel", "I")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_maxEndpoints_field, "maxEndpoints", "I")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_endpointRDOThreshold_field, "endpointRDOThreshold", "F")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_maxSelectors_field, "maxSelectors", "I")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_selectorRDOThreshold_field, "selectorRDOThreshold", "F")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_inputSwizzle_field, "inputSwizzle", "[C")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_normalMap_field, "normalMap", "Z")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_preSwizzle_field, "preSwizzle", "Z")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_noEndpointRDO_field, "noEndpointRDO", "Z")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_noSelectorRDO_field, "noSelectorRDO", "Z")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_uastcFlags_field, "uastcFlags", "I")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_uastcRDO_field, "uastcRDO", "Z")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_uastcRDOQualityScalar_field, "uastcRDOQualityScalar", "F")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_uastcRDODictSize_field, "uastcRDODictSize", "I")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_uastcRDOMaxSmoothBlockErrorScale_field, "uastcRDOMaxSmoothBlockErrorScale", "F")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_uastcRDOMaxSmoothBlockStdDev_field, "uastcRDOMaxSmoothBlockStdDev", "F")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_uastcRDODontFavorSimplerModes_field, "uastcRDODontFavorSimplerModes", "Z")) return JNI_ERR;
    if (!initField(env, cls, KtxBasisParams_uastcRDONoMultithreading_field, "uastcRDONoMultithreading", "Z")) return JNI_ERR;

     return JNI_VERSION_1_4;
}

bool initClass(JNIEnv *env, jclass& cls, const char *name)
{
    cls = env->FindClass(name);
    if (cls == NULL)
    {
        std::cerr << "Failed to initialize class " << name << std::endl;
        return false;
    }
    return true;
}

bool initField(JNIEnv *env, jclass cls, jfieldID& field, const char *name, const char *signature)
{
    field = env->GetFieldID(cls, name, signature);
    if (field == NULL)
    {
        std::cerr << "Failed to initialize field " << name << std::endl;
        return false;
    }
    return true;
}

bool initMethod(JNIEnv *env, jclass cls, jmethodID& method, const char *name, const char *signature)
{
    method = env->GetMethodID(cls, name, signature);
    if (method == NULL)
    {
        std::cerr << "Failed to initialize method " << name << std::endl;
        return false;
    }
    return true;
}


ktxTexture *get_ktx_texture(JNIEnv *env, jobject thiz)
{
    return reinterpret_cast<ktxTexture*>(env->GetLongField(thiz, KtxTexture_instance_field));
}

void set_ktx_texture(JNIEnv *env, jobject thiz, ktxTexture *texture)
{
    env->SetLongField(thiz,
                        KtxTexture_instance_field,
                        reinterpret_cast<jlong>(texture));
}

jobject make_ktx1_wrapper(JNIEnv *env, ktxTexture1 *texture)
{
    return env->NewObject(KtxTexture1_class, KtxTexture1_constructor, reinterpret_cast<jlong>(texture));
}

jobject make_ktx2_wrapper(JNIEnv *env, ktxTexture2 *texture)
{
    return env->NewObject(KtxTexture2_class, KtxTexture2_constructor, reinterpret_cast<jlong>(texture));
}

void copy_ktx_texture_create_info(JNIEnv *env, jobject info, ktxTextureCreateInfo &out)
{
    out.glInternalformat = env->GetIntField(info, KtxTextureCreateInfo_glInternalformat_field);
    out.baseWidth = env->GetIntField(info, KtxTextureCreateInfo_baseWidth_field);
    out.baseHeight = env->GetIntField(info, KtxTextureCreateInfo_baseHeight_field);
    out.baseDepth = env->GetIntField(info, KtxTextureCreateInfo_baseDepth_field);
    out.numDimensions = env->GetIntField(info, KtxTextureCreateInfo_numDimensions_field);
    out.numLevels = env->GetIntField(info, KtxTextureCreateInfo_numLevels_field);
    out.numLayers = env->GetIntField(info, KtxTextureCreateInfo_numLayers_field);
    out.numFaces = env->GetIntField(info, KtxTextureCreateInfo_numFaces_field);
    out.isArray = env->GetBooleanField(info, KtxTextureCreateInfo_isArray_field) ? KTX_TRUE : KTX_FALSE;
    out.generateMipmaps = env->GetBooleanField(info, KtxTextureCreateInfo_generateMipmaps_field) ? KTX_TRUE : KTX_FALSE;
    out.vkFormat = env->GetIntField(info, KtxTextureCreateInfo_vkFormat_field);
}

bool copy_ktx_astc_params(JNIEnv *env, jobject params, ktxAstcParams &out)
{
    // Undocumented quirk!
    out.structSize = sizeof(ktxAstcParams);

    out.verbose = env->GetBooleanField(params, KtxAstcParams_verbose_field);
    out.threadCount = env->GetIntField(params, KtxAstcParams_threadCount_field);
    out.blockDimension = env->GetIntField(params, KtxAstcParams_blockDimension_field);
    out.mode = env->GetIntField(params, KtxAstcParams_mode_field);
    out.qualityLevel = env->GetIntField(params, KtxAstcParams_qualityLevel_field);
    out.normalMap = env->GetBooleanField(params, KtxAstcParams_normalMap_field);
    out.perceptual = env->GetBooleanField(params, KtxAstcParams_perceptual_field);

    jobject inputSwizzleObject = env->GetObjectField(params, KtxAstcParams_inputSwizzle_field);
    jcharArray inputSwizzleArray = static_cast<jcharArray>(inputSwizzleObject);
    jchar *inputSwizzleValues = env->GetCharArrayElements( inputSwizzleArray, NULL);
    if (inputSwizzleValues == NULL) 
    {
        // OutOfMemoryError is already pending
        return false;
    }
    for (int i=0; i<4; i++)
    {
      out.inputSwizzle[i] = static_cast<char>(inputSwizzleValues[i]);
    }
    env->ReleaseCharArrayElements(inputSwizzleArray, inputSwizzleValues, JNI_ABORT);
    return true;
}

bool copy_ktx_basis_params(JNIEnv *env, jobject params, ktxBasisParams &out)
{
    // Undocumented quirk!
    out.structSize = sizeof(ktxBasisParams);

    out.uastc = env->GetBooleanField(params, KtxBasisParams_uastc_field);
    out.verbose = env->GetBooleanField(params, KtxBasisParams_verbose_field);
    out.noSSE = env->GetBooleanField(params, KtxBasisParams_noSSE_field);
    out.threadCount = env->GetIntField(params, KtxBasisParams_threadCount_field);
    out.compressionLevel = env->GetIntField(params, KtxBasisParams_compressionLevel_field);
    out.qualityLevel = env->GetIntField(params, KtxBasisParams_qualityLevel_field);
    out.maxEndpoints = env->GetIntField(params, KtxBasisParams_maxEndpoints_field);
    out.endpointRDOThreshold = env->GetFloatField(params, KtxBasisParams_endpointRDOThreshold_field);
    out.maxSelectors = env->GetIntField(params, KtxBasisParams_maxSelectors_field);
    out.selectorRDOThreshold = env->GetFloatField(params, KtxBasisParams_selectorRDOThreshold_field);

    jobject inputSwizzleObject = env->GetObjectField(params, KtxBasisParams_inputSwizzle_field);
    jcharArray inputSwizzleArray = static_cast<jcharArray>(inputSwizzleObject);
    jchar *inputSwizzleValues = env->GetCharArrayElements( inputSwizzleArray, NULL);
    if (inputSwizzleValues == NULL) 
    {
        // OutOfMemoryError is already pending
        return false;
    }
    for (int i=0; i<4; i++)
    {
      out.inputSwizzle[i] = static_cast<char>(inputSwizzleValues[i]);
    }
    env->ReleaseCharArrayElements(inputSwizzleArray, inputSwizzleValues, JNI_ABORT);

    out.normalMap = env->GetBooleanField(params, KtxBasisParams_normalMap_field);
    out.preSwizzle = env->GetBooleanField(params, KtxBasisParams_preSwizzle_field);
    out.noEndpointRDO = env->GetBooleanField(params, KtxBasisParams_noEndpointRDO_field);
    out.noSelectorRDO = env->GetBooleanField(params, KtxBasisParams_noSelectorRDO_field);
    out.uastcRDO = env->GetBooleanField(params, KtxBasisParams_uastcRDO_field);
    out.uastcFlags = env->GetIntField(params, KtxBasisParams_uastcFlags_field);
    out.uastcRDOQualityScalar = env->GetFloatField(params, KtxBasisParams_uastcRDOQualityScalar_field);
    out.uastcRDODictSize = env->GetIntField(params, KtxBasisParams_uastcRDODictSize_field);
    out.uastcRDOMaxSmoothBlockErrorScale = env->GetFloatField(params, KtxBasisParams_uastcRDOMaxSmoothBlockErrorScale_field);
    out.uastcRDOMaxSmoothBlockStdDev = env->GetFloatField(params, KtxBasisParams_uastcRDOMaxSmoothBlockStdDev_field);
    out.uastcRDODontFavorSimplerModes = env->GetBooleanField(params, KtxBasisParams_uastcRDODontFavorSimplerModes_field);
    out.uastcRDONoMultithreading = env->GetBooleanField(params, KtxBasisParams_uastcRDONoMultithreading_field);

    return true;
}

bool getBufferData(JNIEnv *env, jobject buffer, jbyte** baseAddress, jbyte **actualAddress, jint *length) {

  // Obtain the position and limit of the buffer
  jint position = env->CallIntMethod(buffer, Buffer_position_method);
  if (env->ExceptionCheck()) {
    return false;
  }
  jint limit = env->CallIntMethod(buffer, Buffer_limit_method);
  if (env->ExceptionCheck()) {
    return false;
  }

  // If the buffer is direct, then compute the results from
  // the direct buffer address
  jboolean isDirect = env->CallBooleanMethod(buffer, Buffer_isDirect_method);
  if (env->ExceptionCheck()) {
    return false;
  }
  if (isDirect == JNI_TRUE) {
    void* address = env->GetDirectBufferAddress(buffer);
    if (env->ExceptionCheck()) {
      return false;
    }
    jbyte* start = static_cast<jbyte*>(address);
    *baseAddress = start;
    *actualAddress = start + position;
    *length = (limit - position);
    return true;
  }

  // If the buffer is backed by an array, then compute the
  // results from the array elements. (These have to be 
  // released in the releaseBufferData call!)
  jboolean hasArray = env->CallBooleanMethod(buffer, Buffer_hasArray_method);
  if (env->ExceptionCheck()) {
    return false;
  }
  if (hasArray == JNI_TRUE) {
    jobject array = env->CallObjectMethod(buffer, Buffer_array_method);
    if (env->ExceptionCheck()) {
      return false;
    }
    jbyte* start = env->GetByteArrayElements(static_cast<jbyteArray>(array), NULL);
    if (start == NULL) 
    {
      // OutOfMemoryError is already pending
      return false;
    }
    *baseAddress = start;
    *actualAddress = start + position;
    *length = (limit - position);
    return true;
  }

  return false;
}

void releaseBufferData(JNIEnv *env, jobject buffer, jbyte* baseAddress) {

  // For direct buffers, nothing has to be done
  jboolean isDirect = env->CallBooleanMethod(buffer, Buffer_isDirect_method);
  if (env->ExceptionCheck()) {
    return;
  }
  if (isDirect == JNI_FALSE) {
    return;
  }

  // For array-backed buffers, the elements are released, without
  // writing back any changes
  jboolean hasArray = env->CallBooleanMethod(buffer, Buffer_hasArray_method);
  if (env->ExceptionCheck()) {
    return;
  }
  if (hasArray == JNI_TRUE) {
    jobject array = env->CallObjectMethod(buffer, Buffer_array_method);
    env->ReleaseByteArrayElements(static_cast<jbyteArray>(array), baseAddress, JNI_ABORT);
  }
}

void ThrowByName(JNIEnv *env, const char *name, const char *msg)
{
    jclass cls = env->FindClass(name);
    if (cls != NULL)
    {
        env->ThrowNew(cls, msg);
    }
    env->DeleteLocalRef(cls);
}

void ThrowDestroyed(JNIEnv *env) 
{
    ThrowByName(env, "java/lang/IllegalStateException", "Cannot use a texture after destroy() was called");
}

