# CMake generated Testfile for 
# Source directory: C:/Users/Shuka/source/repos/KhronosGroup/KTX-Software/interface/python_binding
# Build directory: C:/Users/Shuka/source/repos/KhronosGroup/KTX-Software/out/build/x64-Debug/interface/python_binding
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(pyktx "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" "-E" "env" "LIBKTX_INCLUDE_DIR=C:/Users/Shuka/source/repos/KhronosGroup/KTX-Software/include" "LIBKTX_LIB_DIR=C:/Users/Shuka/source/repos/KhronosGroup/KTX-Software/out/build/x64-Debug/Debug" "KTX_RUN_TESTS=ON" "DYLD_LIBRARY_PATH=C:/Users/Shuka/source/repos/KhronosGroup/KTX-Software/out/build/x64-Debug/Debug:" "LD_LIBRARY_PATH=C:/Users/Shuka/source/repos/KhronosGroup/KTX-Software/out/build/x64-Debug/Debug:" "C:\\Users\\Shuka\\AppData\\Local\\Microsoft\\WindowsApps\\PythonSoftwareFoundation.Python.3.11_qbz5n2kfra8p0\\python.exe" "buildscript.py")
set_tests_properties(pyktx PROPERTIES  WORKING_DIRECTORY "C:/Users/Shuka/source/repos/KhronosGroup/KTX-Software/interface/python_binding" _BACKTRACE_TRIPLES "C:/Users/Shuka/source/repos/KhronosGroup/KTX-Software/interface/python_binding/CMakeLists.txt;96;add_test;C:/Users/Shuka/source/repos/KhronosGroup/KTX-Software/interface/python_binding/CMakeLists.txt;0;")
