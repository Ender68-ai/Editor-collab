@echo off
:: 1. Find and initialize the Visual Studio x64 Native Tools Environment
:: This path is the default for VS 2022 Community. Update if yours is different.
set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

if exist "%VS_PATH%" (
    call "%VS_PATH%"
) else (
    echo [ERROR] could not find vcvars64.bat at %VS_PATH%
    pause
    exit /b
)

:: 2. Execute your specific build commands
echo [STATUS] Cleaning and Building...

rd /s /q build 
geode build --ninja -- -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++

:: 3. Keep the window open for further commands
echo.
echo [DONE] Build process finished.
cmd /k