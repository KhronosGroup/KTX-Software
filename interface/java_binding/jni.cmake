# Copyright (c) 2021, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0


if(APPLE)
    execute_process(COMMAND /usr/libexec/java_home
                    OUTPUT_VARIABLE JAVA_HOME
                    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    set(JAVA_HOME ENV{JAVA_HOME})
endif()
if (JAVA_HOME)
    include_directories($ENV{JAVA_HOME}/include)

    if (APPLE)
        include_directories(${JAVA_HOME}/include/darwin)
    elseif(WIN32)
        include_directories(${JAVA_HOME}/include/win32)
    else()
        include_directories(${JAVA_HOME}/include/linux)
    endif()

    add_library(ktx-jni SHARED
        interface/java_binding/src/main/cpp/KtxTexture.cpp
        interface/java_binding/src/main/cpp/KtxTexture1.cpp
        interface/java_binding/src/main/cpp/KtxTexture2.cpp
        interface/java_binding/src/main/cpp/libktx-jni.cpp
        interface/java_binding/jni.cmake
    )

    set_target_properties(ktx-jni PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
        #INSTALL_RPATH "@executable_path;/usr/local/lib"
    )
    set_xcode_code_sign(ktx-jni)

    if(APPLE AND KTX_EMBED_BITCODE)
        target_compile_options(ktx-jni PRIVATE "-fembed-bitcode")
    endif()

    target_include_directories(ktx-jni PRIVATE include)

    target_link_libraries(ktx-jni ktx)

    install(TARGETS ktx-jni
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT jni
    )
else()
    message(FATAL_ERROR "JAVA_HOME is not set with KTX_FEATURE_JNI enabled! Turn it off to skip this.")
endif()
