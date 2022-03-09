/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include "libktx-jni.h"

#include <iostream>

ktxTexture *get_ktx_texture(JNIEnv *env, jobject thiz)
{
    jclass ktx_texture_class = env->GetObjectClass(thiz);
    jfieldID ktx_instance_field = env->GetFieldID(ktx_texture_class, "instance", "J");

    return reinterpret_cast<ktxTexture*>(env->GetLongField(thiz, ktx_instance_field));
}

void set_ktx_texture(JNIEnv *env, jobject thiz, ktxTexture *texture)
{
    jclass ktx_texture_class = env->GetObjectClass(thiz);
    jfieldID ktx_instance_field = env->GetFieldID(ktx_texture_class, "instance", "J");

    env->SetLongField(thiz,
                        ktx_instance_field,
                        reinterpret_cast<jlong>(texture));
}

jobject make_ktx1_wrapper(JNIEnv *env, ktxTexture1 *texture)
{
    jclass ktx_texture_class = env->FindClass("org/khronos/ktx/KtxTexture1");
    assert (ktx_texture_class != NULL);

    jmethodID ktx_texture_ctor = env->GetMethodID(ktx_texture_class, "<init>", "(J)V");

    return env->NewObject(ktx_texture_class, ktx_texture_ctor, reinterpret_cast<jlong>(texture));
}

jobject make_ktx2_wrapper(JNIEnv *env, ktxTexture2 *texture)
{
    jclass ktx_texture_class = env->FindClass("org/khronos/ktx/KtxTexture2");
    assert (ktx_texture_class != NULL);

    jmethodID ktx_texture_ctor = env->GetMethodID(ktx_texture_class, "<init>", "(J)V");

    return env->NewObject(ktx_texture_class, ktx_texture_ctor, reinterpret_cast<jlong>(texture));
}

void copy_ktx_texture_create_info(JNIEnv *env, jobject info, ktxTextureCreateInfo &out)
{
    jclass ktx_info_class = env->GetObjectClass(info);
    jfieldID f_gl_internalformat = env->GetFieldID(ktx_info_class, "glInternalformat", "I");
    jfieldID f_base_width = env->GetFieldID(ktx_info_class, "baseWidth", "I");
    jfieldID f_base_height = env->GetFieldID(ktx_info_class, "baseHeight", "I");
    jfieldID f_base_depth = env->GetFieldID(ktx_info_class, "baseDepth", "I");
    jfieldID f_num_dimensions = env->GetFieldID(ktx_info_class, "numDimensions", "I");
    jfieldID f_num_levels = env->GetFieldID(ktx_info_class, "numLevels", "I");
    jfieldID f_num_layers = env->GetFieldID(ktx_info_class, "numLayers", "I");
    jfieldID f_num_faces = env->GetFieldID(ktx_info_class, "numFaces", "I");
    jfieldID f_is_array = env->GetFieldID(ktx_info_class, "isArray", "Z");
    jfieldID f_generate_mipmaps = env->GetFieldID(ktx_info_class, "generateMipmaps", "Z");
    jfieldID f_vk_format = env->GetFieldID(ktx_info_class, "vkFormat", "I");

    out.glInternalformat = env->GetIntField(info, f_gl_internalformat);
    out.baseWidth = env->GetIntField(info, f_base_width);
    out.baseHeight = env->GetIntField(info, f_base_height);
    out.baseDepth = env->GetIntField(info, f_base_depth);
    out.numDimensions = env->GetIntField(info, f_num_dimensions);
    out.numLevels = env->GetIntField(info, f_num_levels);
    out.numLayers = env->GetIntField(info, f_num_layers);
    out.numFaces = env->GetIntField(info, f_num_faces);
    out.isArray = env->GetBooleanField(info, f_is_array) ? KTX_TRUE : KTX_FALSE;
    out.generateMipmaps = env->GetBooleanField(info, f_generate_mipmaps) ? KTX_TRUE : KTX_FALSE;
    out.vkFormat = env->GetIntField(info, f_vk_format);
}

void copy_ktx_astc_params(JNIEnv *env, jobject params, ktxAstcParams &out)
{
    // Undocumented quirk!
    out.structSize = sizeof(ktxAstcParams);

    jclass ktx_astc_params_class = env->GetObjectClass(params);
    jfieldID verbose = env->GetFieldID(ktx_astc_params_class, "verbose", "Z");
    jfieldID threadCount = env->GetFieldID(ktx_astc_params_class, "threadCount", "I");
    jfieldID blockDimension = env->GetFieldID(ktx_astc_params_class, "blockDimension", "I");
    jfieldID mode = env->GetFieldID(ktx_astc_params_class, "mode", "I");
    jfieldID qualityLevel = env->GetFieldID(ktx_astc_params_class, "qualityLevel", "I");
    jfieldID normalMap = env->GetFieldID(ktx_astc_params_class, "normalMap", "Z");
    jfieldID perceptual = env->GetFieldID(ktx_astc_params_class, "perceptual", "Z");
    jfieldID inputSwizzle = env->GetFieldID(ktx_astc_params_class, "inputSwizzle", "[C");

    out.verbose = env->GetBooleanField(params, verbose);
    out.threadCount = env->GetIntField(params, threadCount);
    out.blockDimension = env->GetIntField(params, blockDimension);
    out.mode = env->GetIntField(params, mode);
    out.qualityLevel = env->GetIntField(params, qualityLevel);
    out.normalMap = env->GetBooleanField(params, normalMap);
    out.perceptual = env->GetBooleanField(params, perceptual);

    env->GetByteArrayRegion(
        static_cast<jbyteArray>(env->GetObjectField(params, inputSwizzle)),
        0,
        4,
        reinterpret_cast<jbyte*>(&out.inputSwizzle)
    );
}

void copy_ktx_basis_params(JNIEnv *env, jobject params, ktxBasisParams &out)
{
    // Undocumented quirk!
    out.structSize = sizeof(ktxBasisParams);

    jclass ktx_basis_params_class = env->GetObjectClass(params);
    jfieldID uastc = env->GetFieldID(ktx_basis_params_class, "uastc", "Z");
    jfieldID verbose = env->GetFieldID(ktx_basis_params_class, "verbose", "Z");
    jfieldID noSSE = env->GetFieldID(ktx_basis_params_class, "noSSE", "Z");
    jfieldID threadCount = env->GetFieldID(ktx_basis_params_class, "threadCount", "I");
    jfieldID compressionLevel = env->GetFieldID(ktx_basis_params_class, "compressionLevel", "I");
    jfieldID qualityLevel = env->GetFieldID(ktx_basis_params_class, "qualityLevel", "I");
    jfieldID maxEndpoints = env->GetFieldID(ktx_basis_params_class, "maxEndpoints", "I");
    jfieldID endpointRDOThreshold = env->GetFieldID(ktx_basis_params_class, "endpointRDOThreshold", "F");
    jfieldID maxSelectors = env->GetFieldID(ktx_basis_params_class, "maxSelectors", "I");
    jfieldID selectorRDOThreshold = env->GetFieldID(ktx_basis_params_class, "selectorRDOThreshold", "F");
    jfieldID inputSwizzle = env->GetFieldID(ktx_basis_params_class, "inputSwizzle", "[C");
    jfieldID normalMap = env->GetFieldID(ktx_basis_params_class, "normalMap", "Z");
    jfieldID preSwizzle = env->GetFieldID(ktx_basis_params_class, "preSwizzle", "Z");
    jfieldID noEndpointRDO = env->GetFieldID(ktx_basis_params_class, "noEndpointRDO", "Z");
    jfieldID noSelectorRDO = env->GetFieldID(ktx_basis_params_class, "noSelectorRDO", "Z");
    jfieldID uastcFlags = env->GetFieldID(ktx_basis_params_class, "uastcFlags", "I");
    jfieldID uastcRDO = env->GetFieldID(ktx_basis_params_class, "uastcRDO", "Z");
    jfieldID uastcRDOQualityScalar = env->GetFieldID(ktx_basis_params_class, "uastcRDOQualityScalar", "F");
    jfieldID uastcRDODictSize = env->GetFieldID(ktx_basis_params_class, "uastcRDODictSize", "I");
    jfieldID uastcRDOMaxSmoothBlockErrorScale = env->GetFieldID(ktx_basis_params_class, "uastcRDOMaxSmoothBlockErrorScale", "F");
    jfieldID uastcRDOMaxSmoothBlockStdDev = env->GetFieldID(ktx_basis_params_class, "uastcRDOMaxSmoothBlockStdDev", "F");
    jfieldID uastcRDODontFavorSimplerModes = env->GetFieldID(ktx_basis_params_class, "uastcRDODontFavorSimplerModes", "Z");
    jfieldID uastcRDONoMultithreading = env->GetFieldID(ktx_basis_params_class, "uastcRDONoMultithreading", "Z");

    out.uastc = env->GetBooleanField(params, uastc);
    out.verbose = env->GetBooleanField(params, verbose);
    out.noSSE = env->GetBooleanField(params, noSSE);
    out.threadCount = env->GetIntField(params, threadCount);
    out.compressionLevel = env->GetIntField(params, compressionLevel);
    out.qualityLevel = env->GetIntField(params, qualityLevel);
    out.maxEndpoints = env->GetIntField(params, maxEndpoints);
    out.endpointRDOThreshold = env->GetFloatField(params, endpointRDOThreshold);
    out.maxSelectors = env->GetIntField(params, maxSelectors);
    out.selectorRDOThreshold = env->GetFloatField(params, selectorRDOThreshold);
    env->GetByteArrayRegion(
        static_cast<jbyteArray>(env->GetObjectField(params, inputSwizzle)),
        0,
        4,
        reinterpret_cast<jbyte*>(&out.inputSwizzle)
    );
    out.normalMap = env->GetBooleanField(params, normalMap);
    out.preSwizzle = env->GetBooleanField(params, preSwizzle);
    out.noEndpointRDO = env->GetBooleanField(params, noEndpointRDO);
    out.noSelectorRDO = env->GetBooleanField(params, noSelectorRDO);
    out.uastcRDO = env->GetBooleanField(params, uastcRDO);
    out.uastcFlags = env->GetIntField(params, uastcFlags);
    out.uastcRDOQualityScalar = env->GetFloatField(params, uastcRDOQualityScalar);
    out.uastcRDODictSize = env->GetIntField(params, uastcRDODictSize);
    out.uastcRDOMaxSmoothBlockErrorScale = env->GetFloatField(params, uastcRDOMaxSmoothBlockErrorScale);
    out.uastcRDOMaxSmoothBlockStdDev = env->GetFloatField(params, uastcRDOMaxSmoothBlockStdDev);
    out.uastcRDODontFavorSimplerModes = env->GetBooleanField(params, uastcRDODontFavorSimplerModes);
    out.uastcRDONoMultithreading = env->GetBooleanField(params, uastcRDONoMultithreading);
}
