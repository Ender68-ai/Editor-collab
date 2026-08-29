cmake_minimum_required(VERSION 3.20)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(MultiplayerEdit VERSION 0.5.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(_ITERATOR_DEBUG_LEVEL=0)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_ITERATOR_DEBUG_LEVEL=0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_ITERATOR_DEBUG_LEVEL=0")

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

if (WIN32)
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX _WINSOCKAPI_)
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src)

set(CMAKE_SKIP_INSTALL_RULES ON)

include(FetchContent)

FetchContent_Declare(
    mbedtls
    GIT_REPOSITORY https://github.com/Mbed-TLS/mbedtls.git
    GIT_TAG        v3.6.0
)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "" FORCE)

FetchContent_GetProperties(mbedtls)
if(NOT mbedtls_POPULATED)
    FetchContent_MakeAvailable(mbedtls)

    set(MBEDTLS_CONFIG_H "${mbedtls_SOURCE_DIR}/include/mbedtls/mbedtls_config.h")
    if(EXISTS "${MBEDTLS_CONFIG_H}")
        file(READ "${MBEDTLS_CONFIG_H}" MBEDTLS_CONFIG_CONTENTS)
        string(REPLACE "//#define MBEDTLS_SSL_DTLS_SRTP"
                       "#define MBEDTLS_SSL_DTLS_SRTP"
                       MBEDTLS_CONFIG_CONTENTS "${MBEDTLS_CONFIG_CONTENTS}")
        file(WRITE "${MBEDTLS_CONFIG_H}" "${MBEDTLS_CONFIG_CONTENTS}")
    endif()

    set(MBEDTLS_CMAKE "${mbedtls_SOURCE_DIR}/CMakeLists.txt")
    if(EXISTS "${MBEDTLS_CMAKE}")
        file(READ "${MBEDTLS_CMAKE}" MBEDTLS_CMAKE_CONTENTS)
        string(REGEX REPLACE "cmake_minimum_required\\(VERSION [0-9.]+\\)"
                             "cmake_minimum_required(VERSION 3.10)"
                             MBEDTLS_CMAKE_CONTENTS "${MBEDTLS_CMAKE_CONTENTS}")
        file(WRITE "${MBEDTLS_CMAKE}" "${MBEDTLS_CMAKE_CONTENTS}")
    endif()

endif()

if (TARGET mbedtls AND NOT TARGET MbedTLS::MbedTLS)
    add_library(MbedTLS::MbedTLS ALIAS mbedtls)
endif()

FetchContent_Declare(
    libdatachannel
    GIT_REPOSITORY https://github.com/paullouisageneau/libdatachannel.git
    GIT_TAG        v0.22.4
)

set(NO_MEDIA ON CACHE BOOL "" FORCE)
set(NO_WEBSOCKET OFF CACHE BOOL "" FORCE)
set(NO_EXAMPLES ON CACHE BOOL "" FORCE)
set(NO_TESTS ON CACHE BOOL "" FORCE)

set(USE_GNUTLS OFF CACHE BOOL "" FORCE)
set(USE_NICE OFF CACHE BOOL "" FORCE)
set(PREFER_SYSTEM_LIB OFF CACHE BOOL "" FORCE)

set(USE_OPENSSL OFF CACHE BOOL "" FORCE)
set(USE_MBEDTLS ON CACHE BOOL "" FORCE)

FetchContent_GetProperties(libdatachannel)
if(NOT libdatachannel_POPULATED)
    FetchContent_MakeAvailable(libdatachannel)

    set(USRSCTP_CMAKE "${libdatachannel_SOURCE_DIR}/deps/usrsctp/CMakeLists.txt")
    if(EXISTS "${USRSCTP_CMAKE}")
        file(READ "${USRSCTP_CMAKE}" USRSCTP_CONTENTS)
        string(REPLACE "if (CMAKE_C_COMPILER_ID MATCHES \"MSVC\" OR CMAKE_C_SIMULATE_ID MATCHES \"MSVC\")"
                       "if ((CMAKE_C_COMPILER_ID MATCHES \"MSVC\" OR CMAKE_C_SIMULATE_ID MATCHES \"MSVC\") AND NOT CMAKE_C_COMPILER_ID MATCHES \"Clang\")"
                       USRSCTP_CONTENTS "${USRSCTP_CONTENTS}")
        file(WRITE "${USRSCTP_CMAKE}" "${USRSCTP_CONTENTS}")
    endif()

    foreach(dep plog usrsctp libjuice)
        set(DEP_CMAKE "${libdatachannel_SOURCE_DIR}/deps/${dep}/CMakeLists.txt")
        if(EXISTS "${DEP_CMAKE}")
            file(READ "${DEP_CMAKE}" DEP_CONTENTS)
            string(REGEX REPLACE "cmake_minimum_required\\(VERSION [0-9.]+\\)"
                                 "cmake_minimum_required(VERSION 3.10)"
                                 DEP_CONTENTS "${DEP_CONTENTS}")
            file(WRITE "${DEP_CMAKE}" "${DEP_CONTENTS}")
        endif()
    endforeach()

    set(SCTP_CPP "${libdatachannel_SOURCE_DIR}/src/impl/sctptransport.cpp")
    if(EXISTS "${SCTP_CPP}")
        file(READ "${SCTP_CPP}" SCTP_CONTENTS)
        string(REPLACE "throw std::runtime_error(\"Sending failed, errno=\" + std::to_string(errno));"
                       "return false;"
                       SCTP_CONTENTS "${SCTP_CONTENTS}")
        file(WRITE "${SCTP_CPP}" "${SCTP_CONTENTS}")
    endif()

    set(DC_CPP "${libdatachannel_SOURCE_DIR}/src/impl/datachannel.cpp")
    if(EXISTS "${DC_CPP}")
        file(READ "${DC_CPP}" DC_CONTENTS)
        string(REPLACE "if (!mStream.has_value())\n\t\tthrow std::logic_error(\"DataChannel has no stream assigned\");"
                       "if (!mStream.has_value())\n\t\treturn;"
                       DC_CONTENTS "${DC_CONTENTS}")
        string(REPLACE "throw std::runtime_error(\"DataChannel is closed\");"
                       "return false;"
                       DC_CONTENTS "${DC_CONTENTS}")
        string(REPLACE "throw std::runtime_error(\"DataChannel not open\");"
                       "return false;"
                       DC_CONTENTS "${DC_CONTENTS}")
        string(REPLACE "throw std::logic_error(\"DataChannel has no stream assigned\");"
                       "return false;"
                       DC_CONTENTS "${DC_CONTENTS}")
        file(WRITE "${DC_CPP}" "${DC_CONTENTS}")
    endif()

endif()

target_link_libraries(${PROJECT_NAME} LibDataChannel::LibDataChannelStatic)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

set(CMAKE_SKIP_INSTALL_RULES OFF)

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

if (WIN32)
    target_link_options(${PROJECT_NAME} PRIVATE "-Wl,/MAP")
endif()

add_subdirectory(src/include/cue)
add_subdirectory(src/include/advancedlabel)
add_subdirectory(src/include/uibuilder)

target_link_libraries(${PROJECT_NAME}
    cue
    advanced_label
    LibDataChannel::LibDataChannelStatic
)

setup_geode_mod(${PROJECT_NAME})