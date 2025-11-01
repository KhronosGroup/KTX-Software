@echo off
REM ========================================================================
REM KTX-Software Emscripten Build Script
REM Builds optimized ES6 modules (.mjs) for PlayCanvas/Unity/Web
REM ========================================================================

echo.
echo ========================================
echo KTX-Software Emscripten Build
echo ========================================
echo.

REM Check if Emscripten is activated
where emcc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Emscripten not found!
    echo Please activate Emscripten first:
    echo   C:\emsdk\emsdk_env.bat
    echo.
    pause
    exit /b 1
)

echo Emscripten detected:
emcc --version | findstr "emcc"
echo.

REM Build type selection
set BUILD_TYPE=Release
if "%1"=="debug" set BUILD_TYPE=Debug
if "%1"=="Debug" set BUILD_TYPE=Debug

echo Build Type: %BUILD_TYPE%
echo.

REM Configuration options
set ENABLE_GL_UPLOAD=OFF
set ENABLE_VK_UPLOAD=OFF
set ENABLE_WRITE=ON
set ENABLE_TOOLS=OFF
set ENABLE_TESTS=OFF

if "%2"=="minimal" (
    echo [MINIMAL BUILD - Read-only, no GL, smallest size]
    set ENABLE_WRITE=OFF
) else if "%2"=="full" (
    echo [FULL BUILD - All features, larger size]
    set ENABLE_GL_UPLOAD=ON
) else (
    echo [STANDARD BUILD - Read/Write, no GL, optimized]
)
echo.

REM Create build directory
set BUILD_DIR=build-emscripten-%BUILD_TYPE%
if "%2"=="minimal" set BUILD_DIR=%BUILD_DIR%-minimal

echo Creating build directory: %BUILD_DIR%
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%

REM Configure
echo.
echo ----------------------------------------
echo Configuring CMake...
echo ----------------------------------------
emcmake cmake .. ^
  -G Ninja ^
  -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
  -DKTX_FEATURE_GL_UPLOAD=%ENABLE_GL_UPLOAD% ^
  -DKTX_FEATURE_VK_UPLOAD=%ENABLE_VK_UPLOAD% ^
  -DKTX_FEATURE_WRITE=%ENABLE_WRITE% ^
  -DKTX_FEATURE_TOOLS=%ENABLE_TOOLS% ^
  -DKTX_FEATURE_TESTS=%ENABLE_TESTS% ^
  -DKTX_FEATURE_LOADTEST_APPS=OFF

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

REM Build
echo.
echo ----------------------------------------
echo Building...
echo ----------------------------------------
cmake --build . --config %BUILD_TYPE%

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed!
    cd ..
    pause
    exit /b 1
)

REM Success message
echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo.
echo Output files:
dir /b libktx*.mjs 2>nul
dir /b libktx*.wasm 2>nul
dir /b msc_basis_transcoder*.mjs 2>nul
dir /b msc_basis_transcoder*.wasm 2>nul
echo.

REM Show file sizes
echo File sizes:
for %%f in (libktx.mjs libktx.wasm libktx_read.mjs libktx_read.wasm) do (
    if exist %%f (
        for %%A in (%%f) do (
            set size=%%~zA
            set /a sizeKB=%%~zA/1024
            echo   %%f: !sizeKB! KB
        )
    )
)
echo.

echo Files also copied to: ..\tests\webgl\
echo.

cd ..

echo ========================================
echo Usage in your project:
echo ========================================
echo.
echo import createKtxModule from './libktx.mjs';
echo.
echo const ktx = await createKtxModule();
echo await ktx.ready;
echo.
echo const texture = new ktx.texture(ktx2Data);
echo console.log('Levels:', texture.numLevels);
echo console.log('Format:', texture.vkFormat);
echo.
echo For more examples, see tests/webgl/
echo.

pause
