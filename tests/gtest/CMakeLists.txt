add_library( gtest SHARED
    src/gtest-all.cc
    src/gtest-internal-inl.h
    src/gtest_main.cc
)

target_include_directories(
    gtest
    PUBLIC
    include
    .
)
