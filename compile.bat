@echo off
setlocal EnableExtensions

rem Clean build + CMake cache (IMPORTANT fix)
if exist build rd /s /q build

rem =========================
rem MAX RUNTIME FLAGS (Clang)
rem =========================
set "CFLAGS=-O3 -ffast-math -DNDEBUG -march=native -mtune=native -flto"
set "CXXFLAGS=-O3 -ffast-math -DNDEBUG -march=native -mtune=native -flto"

echo Configuring...

cmake -S . -B build -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_C_COMPILER=clang ^
  -DCMAKE_CXX_COMPILER=clang++ ^
  -DCMAKE_C_FLAGS_RELEASE="%CFLAGS%" ^
  -DCMAKE_CXX_FLAGS_RELEASE="%CXXFLAGS%" ^
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON ^
  -DCMAKE_POLICY_DEFAULT_CMP0069=OFF ^
  -DCMAKE_EXE_LINKER_FLAGS_RELEASE="-fuse-ld=lld" ^
  -DCMAKE_SHARED_LINKER_FLAGS_RELEASE="-fuse-ld=lld"

echo Building...

cmake --build build

echo Built successfully.
cmd /k