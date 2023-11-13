if(EXISTS "C:/Users/Shuka/source/repos/KhronosGroup/KTX-Software/out/build/x64-Debug/Debug/texturetests.exe")
  if("C:/Users/Shuka/source/repos/KhronosGroup/KTX-Software/out/build/x64-Debug/Debug/texturetests.exe" IS_NEWER_THAN "C:/Users/Shuka/source/repos/KhronosGroup/KTX-Software/out/build/x64-Debug/tests/texturetests[1]_tests.cmake")
    include("C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/share/cmake-3.20/Modules/GoogleTestAddTests.cmake")
    gtest_discover_tests_impl(
      TEST_EXECUTABLE [==[C:/Users/Shuka/source/repos/KhronosGroup/KTX-Software/out/build/x64-Debug/Debug/texturetests.exe]==]
      TEST_EXECUTOR [==[]==]
      TEST_WORKING_DIR [==[C:/Users/Shuka/source/repos/KhronosGroup/KTX-Software/out/build/x64-Debug/tests]==]
      TEST_EXTRA_ARGS [==[]==]
      TEST_PROPERTIES [==[]==]
      TEST_PREFIX [==[texturetest]==]
      TEST_SUFFIX [==[]==]
      NO_PRETTY_TYPES [==[FALSE]==]
      NO_PRETTY_VALUES [==[FALSE]==]
      TEST_LIST [==[texturetests_TESTS]==]
      CTEST_FILE [==[C:/Users/Shuka/source/repos/KhronosGroup/KTX-Software/out/build/x64-Debug/tests/texturetests[1]_tests.cmake]==]
      TEST_DISCOVERY_TIMEOUT [==[20]==]
      TEST_XML_OUTPUT_DIR [==[]==]
    )
  endif()
  include("C:/Users/Shuka/source/repos/KhronosGroup/KTX-Software/out/build/x64-Debug/tests/texturetests[1]_tests.cmake")
else()
  add_test(texturetests_NOT_BUILT texturetests_NOT_BUILT)
endif()
