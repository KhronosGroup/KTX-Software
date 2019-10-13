
find_package(SDL2)

set( SDL_SRC
    appfwSDL/AppBaseSDL.cpp
    appfwSDL/main.cpp
)

add_executable( glloadtest
    $<TARGET_OBJECTS:objUtil>
    glloadtests/GLLoadTests.cpp
    common/LoadTestSample.cpp
    common/SwipeDetector.cpp
    glloadtests/shader-based/BasisuTest.cpp
    glloadtests/shader-based/DrawTexture.cpp
    glloadtests/shader-based/GL3LoadTestSample.cpp
    glloadtests/shader-based/GL3LoadTests.cpp
    glloadtests/shader-based/TextureArray.cpp
    glloadtests/shader-based/TexturedCube.cpp
    glloadtests/shader-based/shaders.cpp
    ${SDL_SRC}
    appfwSDL/GLAppSDL.cpp
    )

target_include_directories(
    glloadtest
    # PUBLIC
    PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${objKtxRead_include_dirs}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../other_include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib
    ${CMAKE_CURRENT_SOURCE_DIR}/../../utils
    common
    geom
    appfwSDL
    glloadtests
    )

target_link_libraries(
    glloadtest
    ${OPENGL_LIBRARIES}
    ${SDL2_LIBRARIES}
    gtest
    ktx
    )

target_compile_definitions(
    glloadtest
    PRIVATE
    ${KTX_DEFS}
)
