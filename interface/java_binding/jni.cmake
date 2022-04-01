# Copyright (c) 2021, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

find_package(Java REQUIRED)
find_program(MAVEN_EXECUTABLE mvn PATHS $ENV{PATH})

add_library(ktx-jni SHARED
    interface/java_binding/src/main/cpp/KtxTexture.cpp
    interface/java_binding/src/main/cpp/KtxTexture1.cpp
    interface/java_binding/src/main/cpp/KtxTexture2.cpp
    interface/java_binding/src/main/cpp/libktx-jni.cpp
    interface/java_binding/jni.cmake
)

target_include_directories(ktx-jni SYSTEM PRIVATE
      ${_JAVA_HOME}/include
      # Has to be one long line otherwise directory is not set as SYSTEM.
      $<IF:$<BOOL:APPLE>,${_JAVA_HOME}/include/darwin,$<IF:$<BOOL:WIN32>,${_JAVA_HOME}/include/win32,${_JAVA_HOME}/include/linux>>
)

set_target_properties(ktx-jni PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
    INSTALL_RPATH "@rpath;/usr/local/lib"
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

add_custom_command(
    OUTPUT
        ${CMAKE_SOURCE_DIR}/interface/java_binding/target/libktx-${PROJECT_VERSION}-source.jar
        ${CMAKE_SOURCE_DIR}/interface/java_binding/target/libktx-${PROJECT_VERSION}.jar
    COMMAND
        ${MAVEN_EXECUTABLE} -Drevision=${PROJECT_VERSION} -Dmaven.test.skip=true package
    DEPENDS
        ktx-jni
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/interface/java_binding
)

add_custom_target( ktx-jar
    DEPENDS
        ${CMAKE_SOURCE_DIR}/interface/java_binding/target/libktx-${PROJECT_VERSION}-source.jar
        ${CMAKE_SOURCE_DIR}/interface/java_binding/target/libktx-${PROJECT_VERSION}.jar
    WORKING_DIRECTORY
        interface/java_binding
    COMMENT
        "Java wrapper target"
)

install(FILES
    ${CMAKE_SOURCE_DIR}/interface/java_binding/target/libktx-${PROJECT_VERSION}.jar
    TYPE LIB
    COMPONENT jni
)
