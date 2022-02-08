# Copyright (c) 2021, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

if (KTX_FEATURE_JNI)
    if (DEFINED ENV{JAVA_HOME})
        include_directories($ENV{JAVA_HOME}/include)

        if (APPLE)
            include_directories($ENV{JAVA_HOME}/include/darwin)
        elseif(WIN32)
            include_directories($ENV{JAVA_HOME}/include/win32)
        else()
            include_directories($ENV{JAVA_HOME}/include/linux)
        endif()

        add_library(ktx-jni SHARED
            interface/java_binding/src/main/cpp/KtxTexture.cpp
            interface/java_binding/src/main/cpp/KtxTexture1.cpp
            interface/java_binding/src/main/cpp/KtxTexture2.cpp
            interface/java_binding/src/main/cpp/libktx-jni.cpp
        )

        target_include_directories(ktx-jni PRIVATE include)

        target_link_libraries(ktx-jni ktx)

        if(APPLE)
            set_target_properties(ktx-jni PROPERTIES
                XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
                INSTALL_RPATH "@executable_path;/usr/local/lib"
            )
        endif()

        install(TARGETS ktx-jni LIBRARY)
    else()
        message(FATAL_ERROR "JAVA_HOME is not set with KTX_FEATURE_JNI enabled! Turn it off to skip this.")
    endif()
endif()
