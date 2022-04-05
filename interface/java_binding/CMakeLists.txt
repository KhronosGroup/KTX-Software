# Copyright (c) 2021, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0


if(WIN32 AND "$ENV{APPVEYOR}" STREQUAL "True")
    message(STATUS "Setting _JAVA_HOME to $ENV{JAVA_HOME}")
    # This is a helping hand to the FindJava module as by itself, it doesn't
    # find the JVMs on Appveyor. To avoid the rathole I've just been down
    # note that we can't use the PATHS or HINTS parameters of find_package
    # because they are exclusive to "config" mode and there are no cmake
    # config files in the JDK.
    set(_JAVA_HOME $ENV{JAVA_HOME})
endif()
find_package(Java 11 REQUIRED COMPONENTS Development MODULE)
find_program(MAVEN_EXECUTABLE mvn PATHS $ENV{PATH})

add_library(ktx-jni SHARED
    src/main/cpp/KtxTexture.cpp
    src/main/cpp/KtxTexture1.cpp
    src/main/cpp/KtxTexture2.cpp
    src/main/cpp/libktx-jni.cpp
)

target_include_directories(ktx-jni SYSTEM PRIVATE
      ${_JAVA_HOME}/include
      # Has to be one long line otherwise directory is not set as SYSTEM.
      $<IF:$<BOOL:${APPLE}>,${_JAVA_HOME}/include/darwin,$<IF:$<BOOL:${WIN32}>,${_JAVA_HOME}/include/win32,${_JAVA_HOME}/include/linux>>
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

install(TARGETS ktx-jni LIBRARY
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

add_custom_target( ktx-jar ALL
    DEPENDS
        ${CMAKE_SOURCE_DIR}/interface/java_binding/target/libktx-${PROJECT_VERSION}-source.jar
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
