#include <assert.h>
#include <iostream>
#include "libktx-jni.h"

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KTXTexture1_getGlFormat(JNIEnv *env,
                                                                                jobject thiz)
{
    return get_ktx1_texture(env, thiz)->glFormat;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KTXTexture1_getGlInternalformat(JNIEnv *env,
                                                                                jobject thiz)
{
    return get_ktx1_texture(env, thiz)->glInternalformat;
}

extern "C" JNIEXPORT jint JNICALL Java_org_khronos_ktx_KTXTexture1_getGlBaseInternalformat(JNIEnv *env,
                                                                                jobject thiz)
{
    return get_ktx1_texture(env, thiz)->glBaseInternalformat;
}

extern "C" JNIEXPORT jobject JNICALL Java_org_khronos_ktx_KTXTexture1_create(JNIEnv *env,
                                                                                jobject thiz,
                                                                                jobject java_create_info,
                                                                                jint storageAllocation)
{
    ktxTextureCreateInfo info;
    copy_ktx_texture_create_info(env, java_create_info, info);

    ktxTexture1 *instance;
    KTX_error_code result;

    ktxTextureCreateStorageEnum storage_alloc = static_cast<ktxTextureCreateStorageEnum>(storageAllocation);
    result = ktxTexture1_Create(&info, storage_alloc, &instance);

    assert (instance != NULL);

    if (result != KTX_SUCCESS)
    {
        std::cout << "Failure to create KTX1Texture, error " << result << std::endl;
        return NULL;
    }

    jclass ktx_texture_class = env->FindClass("org/khronos/ktx/KTXTexture1");
    assert (ktx_texture_class != NULL);

    jmethodID ktx_texture_ctor = env->GetMethodID(ktx_texture_class, "<init>", "(J)V");
    jobject texture = env->NewObject(ktx_texture_class, ktx_texture_ctor, reinterpret_cast<jlong>(instance));

    return texture;
}
