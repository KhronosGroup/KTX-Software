# Copyright (c) 2021, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

# This ugliness is to workaround that, unless JAVA_HOME is explicitly set,
# FindJNI tries to find a Java framework even though recent Java versions,
# such as we need, are not Frameworks so the search fails. This is still the
# case in CMake 3.19.2. Even if this is fixed in the latest CMake, due to some
# of our CI build environments we can't use it.
if(DEFINED CMAKE_FIND_FRAMEWORK)
    set(SAVED_CMAKE_FIND_FRAMEWORK ${CMAKE_FIND_FRAMEWORK})
endif()
set(CMAKE_FIND_FRAMEWORK NEVER)
find_package(JNI 11 REQUIRED)
if(DEFINED SAVED_CMAKE_FIND_FRAMEWORK)
    set(CMAKE_FIND_FRAMEWORK ${SAVED_CMAKE_FIND_FRAMEWORK})
endif()
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
    # The location of libktx_jni must be set explicitly in java.library.path.
    # This sets places to search for libktx. The second is needed to
    # successfully run the tests during build. Setting "./" here, to say look
    # in the same directory, does not work. @executable_path does not
    # work as the executable here is the JVM.
    INSTALL_RPATH "/usr/local/lib;${KTX_BUILD_DIR}/$<CONFIG>"
)
set_code_sign(ktx-jni)

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
        ${MAVEN_EXECUTABLE} --quiet -Drevision=${PROJECT_VERSION} -Dmaven.test.skip=true package
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
    COMMAND ${MAVEN_EXECUTABLE} --quiet -Drevision=${PROJECT_VERSION} test
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/interface/java_binding
)

set_tests_properties(
    Java-wrapper
PROPERTIES
    ENVIRONMENT _JAVA_OPTIONS=-Djava.library.path=$<TARGET_GENEX_EVAL:ktx-jni,$<TARGET_PROPERTY:ktx-jni,LIBRARY_OUTPUT_DIRECTORY>>
)

# vim:ai:ts=4:sts=4:sw=2:expandtab
