CMAKE_VERSION_REQUIRED(VERSION 3.16)
PROJECT(libktx-jni)

ADD_LIBRARY(ktx-jni SHARED
    KTXTexture.cpp
    KTXTexture1.cpp
    KTXTexture2.cpp
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

TARGET_LINK_LIBRARIES(ktx)
INSTALL(TARGETS ktx-jni LIBRARY)
