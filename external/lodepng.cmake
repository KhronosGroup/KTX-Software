####################################################
# lodepng
####################################################
include(FetchContent)

if (TARGET lodepng::lodepng)
    message(STATUS "(${PROJECT_NAME}): Using configured lodepng target")
    return()
endif()

# Declare package
FetchContent_Declare(
    lodepng
    GIT_REPOSITORY https://github.com/KhronosGroup/lodepng.git
    GIT_TAG d4cc52d8c074da137303e1117cc098a97647960c
)

# Populate lodepng
FetchContent_MakeAvailable(lodepng)

add_library(
    lodepng STATIC 
    ${lodepng_SOURCE_DIR}/lodepng.cpp
    ${lodepng_SOURCE_DIR}/lodepng.h
)
add_library(lodepng::lodepng ALIAS lodepng)
target_include_directories(
    lodepng PUBLIC
    ${lodepng_SOURCE_DIR}
)
