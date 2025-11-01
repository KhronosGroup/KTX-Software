@echo off
setlocal enabledelayedexpansion

REM ========================================================================
REM KTX-Software Emscripten Build Script (Working Version)
REM Builds optimized ES6 modules (.mjs) with size optimization
REM ========================================================================

echo.
echo ========================================
echo KTX-Software Web Build
echo ========================================
echo.

REM Check if Emscripten is activated
where emcc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Emscripten not found!
    echo.
    echo Please run this first:
    echo   C:\emsdk\emsdk_env.bat
    echo.
    echo Then run this script again.
    pause
    exit /b 1
)

echo [OK] Emscripten detected:
emcc --version | findstr "emcc"
echo.

REM Store current directory
set ROOT_DIR=%CD%

REM Build configuration
set BUILD_TYPE=Release
set BUILD_DIR=build-web-release

REM Parse arguments
if "%1"=="debug" (
    set BUILD_TYPE=Debug
    set BUILD_DIR=build-web-debug
    echo Build Type: Debug
) else (
    echo Build Type: Release ^(optimized for size^)
)

if "%1"=="clean" (
    echo.
    echo Cleaning old build directory...
    if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
    echo [OK] Cleaned
)

echo Build Directory: %BUILD_DIR%
echo.

REM Create and enter build directory
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

echo ========================================
echo Step 1: CMake Configuration
echo ========================================
echo.

emcmake cmake -B %BUILD_DIR% -G Ninja ^
  -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
  -DKTX_FEATURE_GL_UPLOAD=OFF ^
  -DKTX_FEATURE_VK_UPLOAD=OFF ^
  -DKTX_FEATURE_WRITE=ON ^
  -DKTX_FEATURE_TOOLS=OFF ^
  -DKTX_FEATURE_TESTS=OFF ^
  -DKTX_FEATURE_LOADTEST_APPS=OFF

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] CMake configuration failed!
    echo.
    pause
    exit /b 1
)

echo.
echo [OK] Configuration completed
echo.

echo ========================================
echo Step 2: Building
echo ========================================
echo.

REM Change to build directory for actual build
cd /d %ROOT_DIR%\%BUILD_DIR%

cmake --build . --config %BUILD_TYPE%

if %ERRORLEVEL% NEQ 0 (
    cd /d %ROOT_DIR%
    echo.
    echo [ERROR] Build failed!
    echo.
    pause
    exit /b 1
)

REM Return to root
cd /d %ROOT_DIR%

echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo.

echo Output files in: %BUILD_DIR%\
echo.

REM Show files with sizes
echo Generated files:
echo ----------------------------------------
for %%f in (
    %BUILD_DIR%\libktx.mjs
    %BUILD_DIR%\libktx.wasm
    %BUILD_DIR%\libktx_read.mjs
    %BUILD_DIR%\libktx_read.wasm
    %BUILD_DIR%\msc_basis_transcoder.mjs
    %BUILD_DIR%\msc_basis_transcoder.wasm
) do (
    if exist %%f (
        for %%A in (%%f) do (
            set /a sizeKB=%%~zA/1024
            set /a sizeMB=%%~zA/1048576
            if !sizeMB! GTR 0 (
                echo   %%~nxf: !sizeMB! MB ^(%%~zA bytes^)
            ) else (
                echo   %%~nxf: !sizeKB! KB ^(%%~zA bytes^)
            )
        )
    )
)
echo.

echo Files also copied to: tests\webgl\
echo.

echo ========================================
echo Quick Start Guide
echo ========================================
echo.
echo 1. Copy files to your project:
echo    copy %BUILD_DIR%\libktx.mjs YourProject\
echo    copy %BUILD_DIR%\libktx.wasm YourProject\
echo.
echo 2. Use in your code:
echo    import createKtxModule from './libktx.mjs';
echo.
echo    const ktx = await createKtxModule^(^);
echo    await ktx.ready;
echo.
echo    const texture = new ktx.texture^(ktx2Data^);
echo    console.log^('Levels:', texture.numLevels^);
echo.
echo See EMSCRIPTEN_USAGE_RU.md for more examples
echo.
echo ========================================
echo Applied Optimizations:
echo ========================================
echo  - Size optimization: -Oz
echo  - ES6 modules: .mjs extension
echo  - No filesystem: -sFILESYSTEM=0
echo  - Closure compiler: --closure=1
echo  - LTO: -flto
echo  - Browser only: -sENVIRONMENT=web,worker
echo ========================================
echo.

pause
