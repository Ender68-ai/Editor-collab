cmake_minimum_required(VERSION 3.26)
project(CollabEditor VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/main_simple.cpp src/CollabManager.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Set output properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "CollabEditor"
)

# Create .geode package directory structure
set(GEODE_PACKAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/CollabEditor.geode")
file(MAKE_DIRECTORY ${GEODE_PACKAGE_DIR})

# Copy files to .geode package after build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${PROJECT_NAME}>
        ${GEODE_PACKAGE_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/mod.json
        ${GEODE_PACKAGE_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/server
        ${GEODE_PACKAGE_DIR}/server
)
