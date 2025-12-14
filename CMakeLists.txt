cmake_minimum_required(VERSION 3.20)
project(FantasySim VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)
option(ENABLE_PROFILING "Enable profiling" OFF)

# Platform detection
if(WIN32)
    set(PLATFORM_WINDOWS ON)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_LINUX ON)
elseif(APPLE)
    set(PLATFORM_MACOS ON)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find packages
# Note: SDL2 packages should be installed via vcpkg
# When using vcpkg, the toolchain file must be set: -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

# Source files
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*.h"
)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries (using modern CMake target-based approach)
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_ttf::SDL2_ttf
)

# Compiler-specific options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX- /permissive-)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# SIMD support
if(ENABLE_SIMD)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /arch:AVX2)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -mavx2 -mfma)
    endif()
endif()

# Profiling
if(ENABLE_PROFILING)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_PROFILING)
endif()

# Copy config files to build directory
file(COPY ${CMAKE_SOURCE_DIR}/config DESTINATION ${CMAKE_BINARY_DIR})

# Copy assets to build directory
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
