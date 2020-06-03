
add_executable( gl3loadtests
${EXE_FLAG}
$<TARGET_OBJECTS:appfwSDL>
$<TARGET_OBJECTS:objUtil>
appfwSDL/GLAppSDL.cpp
common/LoadTestSample.cpp
common/SwipeDetector.cpp
glloadtests/GLLoadTests.cpp
glloadtests/shader-based/BasisuTest.cpp
glloadtests/shader-based/DrawTexture.cpp
glloadtests/shader-based/GL3LoadTests.cpp
glloadtests/shader-based/GL3LoadTestSample.cpp
glloadtests/shader-based/shaders.cpp
glloadtests/shader-based/TextureArray.cpp
glloadtests/shader-based/TextureCubemap.cpp
glloadtests/shader-based/TexturedCube.cpp
${GL_LOAD_TEST_RESOURCE_FILES}
)

target_include_directories(
gl3loadtests
PRIVATE
$<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
${PROJECT_SOURCE_DIR}/lib
${PROJECT_SOURCE_DIR}/other_include
${PROJECT_SOURCE_DIR}/utils
appfwSDL
common
geom
glloadtests
glloadtests/utils
)

if(OPENGL_FOUND)
target_include_directories(
    gl3loadtests
    PRIVATE
    ${OPENGL_INCLUDE_DIR}
)
endif()

target_link_libraries(
gl3loadtests
ktx
)

if(OPENGL_FOUND AND NOT EMSCRIPTEN)
    target_link_libraries(
        gl3loadtests
        ${OPENGL_LIBRARIES}
    )
endif()

if(SDL2_FOUND)
    target_link_libraries(
        gl3loadtests
        ${SDL2_LIBRARIES}
    )
endif()

if(ZLIB_FOUND)
    target_link_libraries( gl3loadtests ZLIB::ZLIB )
endif()

if(APPLE)
if(IOS)
    target_link_libraries(
        gl3loadtests
        "-framework OpenGLES"
        ${CMAKE_SOURCE_DIR}/other_lib/ios/$<CONFIG>-iphoneos/libSDL2.a
        ${CMAKE_SOURCE_DIR}/other_lib/ios/Release-iphoneos/libassimp.a
    )
else()
    target_link_libraries(
        gl3loadtests
        ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libassimp.a
        ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libIrrXML.a
        ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libminizip.a
    )
endif()
elseif(EMSCRIPTEN)
    set_target_properties(
        gl3loadtests
    PROPERTIES
        COMPILE_FLAGS "-Wpedantic -s DISABLE_EXCEPTION_CATCHING=0 -s USE_SDL=2 -s USE_WEBGL2=1 -O0 -g"
        # LINK_FLAGS "--source-map-base ./ --preload-file testimages --exclude-file testimages/genref --exclude-file testimages/*.pgm --exclude-file testimages/*.ppm --exclude-file testimages/*.pam --exclude-file testimages/*.pspimage -s ALLOW_MEMORY_GROWTH=1 -s DISABLE_EXCEPTION_CATCHING=0 -s USE_SDL=2 -s USE_WEBGL2=1 -g4"
        LINK_FLAGS "-s ALLOW_MEMORY_GROWTH=1 -s DISABLE_EXCEPTION_CATCHING=0 -s USE_SDL=2 -s USE_WEBGL2=1 -g"
    )
elseif(LINUX)
target_link_libraries(
    gl3loadtests
    ${assimp_LIBRARIES}
)
elseif(WIN32)
target_link_libraries(
    gl3loadtests
    "${CMAKE_SOURCE_DIR}/other_lib/win/$<CONFIG>-x64/SDL2.lib"
    "${CMAKE_SOURCE_DIR}/other_lib/win/$<CONFIG>-x64/SDL2main.lib"
    "${CMAKE_SOURCE_DIR}/other_lib/win/Release-x64/assimp.lib"
    "${CMAKE_SOURCE_DIR}/other_lib/win/Release-x64/glew32.lib"
)
ensure_runtime_dependencies_windows(gl3loadtests)
endif()

target_compile_definitions(
    gl3loadtests
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
    "GL_CONTEXT_PROFILE=SDL_GL_CONTEXT_PROFILE_CORE"
    GL_CONTEXT_MAJOR_VERSION=3
    GL_CONTEXT_MINOR_VERSION=3
)

set_target_properties( gl3loadtests PROPERTIES
    RESOURCE "${PROJECT_SOURCE_DIR}/icons/mac/ktx_app.icns"
    MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/tests/loadtests/glloadtests/resources/mac/Info.plist"
    MACOSX_BUNDLE_ICON_FILE "ktx_app.icns"
    # Because libassimp is built with bitcode disabled. It's not important unless
    # submitting to the App Store and currently bitcode is optional.
    XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
    XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES"
)

if(EMSCRIPTEN)
    set_target_properties(gl3loadtests PROPERTIES SUFFIX ".html")
endif()
