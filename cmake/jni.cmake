# Copyright (c) 2021, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

if (DEFINED ENV{JAVA_HOME})
    include_directories($ENV{JAVA_HOME}/include)

    if (APPLE)
        include_directories($ENV{JAVA_HOME}/include/darwin)
    elseif(WINDOWS)
        include_directories($ENV{JAVA_HOME}/include/windows)
    else()
        include_directories($ENV{JAVA_HOME}/include/linux)
    endif()

    add_library(ktx-jni SHARED
        interface/java_binding/src/main/cpp/KTXTexture.cpp
        interface/java_binding/src/main/cpp/KTXTexture1.cpp
        interface/java_binding/src/main/cpp/KTXTexture2.cpp
        interface/java_binding/src/main/cpp/libktx-jni.cpp
    )

    target_include_directories(ktx-jni PRIVATE include)

    target_link_libraries(ktx-jni ktx)
    install(TARGETS ktx-jni LIBRARY)
else()
    message(WARNING "$JAVA_HOME is not set, skipping libktx-jni bindings!")
endif()
