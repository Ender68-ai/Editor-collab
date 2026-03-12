@echo off
echo Building Collaborative Editor Mod...

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake (adjust path to Geode as needed)
echo Configuring with CMake...
cmake .. -DCMAKE_TOOLCHAIN_FILE="C:/path/to/geode/cmake/GeodeToolchain.cmake"

REM Build the mod
echo Building...
cmake --build . --config Release

echo Build complete!
echo Check the build/Release directory for the .geode file
pause
