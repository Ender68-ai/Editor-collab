@echo off
setlocal
echo [FAST BUILD] Initializing Developer Environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

echo [FAST BUILD] Compiling Changes Only...
:: We use ninja directly to avoid the slow CMake re-configuration
geode build --ninja

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed. Check the logs above.
    pause
    exit /b %ERRORLEVEL%
)

echo [SUCCESS] Mod compiled! You can open GD now.
pause