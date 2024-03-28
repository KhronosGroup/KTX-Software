/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * Copyright (c) 2024, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBKTX_JNI_H_2B113062
#define LIBKTX_JNI_H_2B113062

#include "ktx.h"
#include <jni.h>

// Utilities for extracting KTXTexture.texture pointer
ktxTexture *get_ktx_texture(JNIEnv *env, jobject thiz);
void set_ktx_texture(JNIEnv *env, jobject thiz, ktxTexture *texture);

jobject make_ktx1_wrapper(JNIEnv *env, ktxTexture1 *texture);
jobject make_ktx2_wrapper(JNIEnv *env, ktxTexture2 *texture);

void copy_ktx_texture_create_info(JNIEnv *env, jobject info, ktxTextureCreateInfo &out);
bool copy_ktx_astc_params(JNIEnv *env, jobject params, ktxAstcParams &out);
bool copy_ktx_basis_params(JNIEnv *env, jobject params, ktxBasisParams &out);

static inline ktxTexture1 *get_ktx1_texture(JNIEnv *env, jobject thiz)
{
    return reinterpret_cast<ktxTexture1*>(get_ktx_texture(env, thiz));
}

static inline ktxTexture2 *get_ktx2_texture(JNIEnv *env, jobject thiz)
{
    return reinterpret_cast<ktxTexture2*>(get_ktx_texture(env, thiz));
}

/**
 * Initialize the given jclass, and return whether
 * the initialization succeeded
 * 
 * @param env The JNI environment pointer
 * @param cls The class
 * @param name The name of the class, e.g. "java/lang/Object"
 * @return Whether the initialization succeeded
 */
bool initClass(JNIEnv *env, jclass& cls, const char *name);

/**
 * Initialize the specified field ID, and return whether
 * the initialization succeeded
 * 
 * @param env The JNI environment pointer
 * @param field The field ID
 * @param name The name of the field
 * @param signature The signature of the field, 
 * "I" for int, "J" for long, or a class like "Ljava.nio.Buffer;"
 * @return Whether the initialization succeeded
 */
bool initField(JNIEnv *env, jclass cls, jfieldID& field, const char *name, const char *signature);

/**
 * Initialize the specified method ID, and return whether
 * the initialization succeeded
 * 
 * @param env The JNI environment pointer
 * @param method The method ID
 * @param name The name of the method
 * @param signature The signature of the method
 * @return Whether the initialization succeeded
 */
bool initMethod(JNIEnv *env, jclass cls, jmethodID& method, const char *name, const char *signature);

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
bool getBufferData(JNIEnv *env, jobject buffer, jbyte** baseAddress, jbyte **actualAddress, jint *length);

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
void releaseBufferData(JNIEnv *env, jobject buffer, jbyte* baseAddress);

/**
 * Throws a new Java Exception that is identified by the given name, e.g.
 * "java/lang/IllegalArgumentException"
 * and contains the given message.
 */
void ThrowByName(JNIEnv *env, const char *name, const char *msg);

/**
 * Throws a new Java Exception that indicates that a KTX texture
 * was used after its 'destroy()' method was called
 */
void ThrowDestroyed(JNIEnv *env);

#endif
