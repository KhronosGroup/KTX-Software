/*
 * Copyright (c) 2021, Shukant Pal and Contributors
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
void copy_ktx_astc_params(JNIEnv *env, jobject params, ktxAstcParams &out);
void copy_ktx_basis_params(JNIEnv *env, jobject params, ktxBasisParams &out);

static inline ktxTexture1 *get_ktx1_texture(JNIEnv *env, jobject thiz)
{
    return reinterpret_cast<ktxTexture1*>(get_ktx_texture(env, thiz));
}

static inline ktxTexture2 *get_ktx2_texture(JNIEnv *env, jobject thiz)
{
    return reinterpret_cast<ktxTexture2*>(get_ktx_texture(env, thiz));
}

#endif
