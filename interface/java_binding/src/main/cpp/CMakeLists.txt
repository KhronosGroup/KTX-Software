/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

CMAKE_MINIMUM_REQUIRED(VERSION 3.16)
PROJECT(libktx-jni)

if (APPLE)
    LINK_DIRECTORIES(/usr/local/lib)
endif()

ADD_LIBRARY(ktx-jni SHARED
    KTXTexture.cpp
    KTXTexture1.cpp
    KTXTexture2.cpp
    libktx-jni.cpp
)

INCLUDE_DIRECTORIES(
    /usr/include
    /usr/local/include
    $ENV{JAVA_HOME}/include
)

if (APPLE)
    INCLUDE_DIRECTORIES($ENV{JAVA_HOME}/include/darwin)
else()
    INCLUDE_DIRECTORIES($ENV{JAVA_HOME}/include/linux)
endif()

TARGET_LINK_LIBRARIES(ktx-jni ktx)
INSTALL(TARGETS ktx-jni LIBRARY)
