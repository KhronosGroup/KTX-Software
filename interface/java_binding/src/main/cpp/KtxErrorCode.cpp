/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libktx-jni.h"
#include <iostream>

extern "C" JNIEXPORT jstring JNICALL Java_org_khronos_ktx_KtxErrorCode_createString(JNIEnv *env, jclass, jint error)
{
  KTX_error_code errorNative = KTX_error_code(error);
  const char* errorStringNative = ktxErrorString(errorNative);

  // If this causes an OutOfMemoryError, then it will return
  // 'null', with the OutOfMemoryError pending
  jstring errorString = env->NewStringUTF(errorStringNative);
  return errorString;
}

