cmake_minimum_required(VERSION 3.26)
project(CollabEditor VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/main_simple.cpp src/CollabManager.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE -DGEODE_EXPORTS)
