# Copyright (c) 2021, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

find_package(JNI 11 REQUIRED COMPONENTS Development)
if(WIN32)
    set(maven_cmd "mvn.cmd")
else()
    set(maven_cmd "mvn")
endif()
find_program(MAVEN_EXECUTABLE ${maven_cmd}
    DOC "Location of `mvn` or (on Windows) `mvn.cmd` executable"
    PATHS $ENV{PATH}
)

add_library(ktx-jni SHARED
    src/main/cpp/KtxTexture.cpp
    src/main/cpp/KtxTexture1.cpp
    src/main/cpp/KtxTexture2.cpp
    src/main/cpp/libktx-jni.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/ktx-jni.manifest
)

configure_file(src/main/cpp/ktx-jni.manifest.in ktx-jni.manifest)

target_include_directories(ktx-jni SYSTEM PRIVATE
      ${JNI_INCLUDE_DIRS}
)

set_target_properties(ktx-jni PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${KTX_BUILD_DIR}/$<CONFIG>
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
    # This serves a couple of purposes. First it sets places where the
    # library is expected to be found and second it is also the LC_RPATH
    # to use when searching for libktx. The first is necessary because the
    # only executable that will ever be searching for this is the JVM whose
    # LC_RPATH we obviously can't set. For that reason including @rpath here
    # is not useful.
    INSTALL_RPATH "/usr/local/lib;${KTX_BUILD_DIR}/$<CONFIG>"
)
set_xcode_code_sign(ktx-jni)

if(APPLE AND KTX_EMBED_BITCODE)
    target_compile_options(ktx-jni PRIVATE "-fembed-bitcode")
endif()

target_include_directories(ktx-jni PRIVATE include)

target_link_libraries(ktx-jni ktx)

if(APPLE OR LINUX)
    install(TARGETS ktx-jni
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT jni
    )
else()
    install(TARGETS ktx-jni LIBRARY
        RUNTIME
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT jni
    )
endif()

add_custom_command(
    OUTPUT
        ${CMAKE_SOURCE_DIR}/interface/java_binding/target/libktx-${PROJECT_VERSION}-sources.jar
        ${CMAKE_SOURCE_DIR}/interface/java_binding/target/libktx-${PROJECT_VERSION}.jar
    COMMAND
        ${MAVEN_EXECUTABLE} -Drevision=${PROJECT_VERSION} -Dmaven.test.skip=true package
    DEPENDS
        ktx-jni
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/interface/java_binding
)

add_custom_target( ktx-jar ALL
    DEPENDS
        ${CMAKE_SOURCE_DIR}/interface/java_binding/target/libktx-${PROJECT_VERSION}-sources.jar
        ${CMAKE_SOURCE_DIR}/interface/java_binding/target/libktx-${PROJECT_VERSION}.jar
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/interface/java_binding
    COMMENT
        "Java wrapper target"
)

install(FILES
    ${CMAKE_SOURCE_DIR}/interface/java_binding/target/libktx-${PROJECT_VERSION}.jar
    TYPE LIB
    COMPONENT jni
)

add_test( NAME Java-wrapper
    COMMAND ${MAVEN_EXECUTABLE} -Drevision=${PROJECT_VERSION} test
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/interface/java_binding
)

set_tests_properties(
    Java-wrapper
PROPERTIES
    ENVIRONMENT _JAVA_OPTIONS=-Djava.library.path=$<TARGET_GENEX_EVAL:ktx-jni,$<TARGET_PROPERTY:ktx-jni,LIBRARY_OUTPUT_DIRECTORY>>
)

# vim:ai:ts=4:sts=4:sw=2:expandtab
