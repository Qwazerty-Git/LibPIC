cmake_minimum_required(VERSION 3.24)

# LibPIC can be used in two ways:
#   - standalone : its own CMake project (host build + unit tests)
#   - embedded   : pulled into a firmware project with add_subdirectory()
# Project-level settings are therefore only applied in the standalone case,
# so that an embedding project keeps control over its toolchain and flags.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(LibPIC C)

    set(CMAKE_C_STANDARD 99)
    set(CMAKE_C_STANDARD_REQUIRED ON)

    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug)
    endif()

    add_compile_options(-Wall -Wextra)
endif()

set(LIBPIC_SOURCES
    src/input.c
    src/input_group.c
    src/input_pattern.c
    src/output.c
    src/output_group.c
    src/output_pattern.c
    src/output_pwm.c
    src/output_pwm_pattern.c
    src/timer.c
    src/trigger.c
)

# The XC8/MPLAB toolchain has no usable archiver rule (xc8-ar needs an explicit
# operation, which the generated CMAKE_C_ARCHIVE_CREATE does not provide), so an
# embedding firmware project builds the objects directly.
option(LIBPIC_OBJECT_LIBRARY "Build libpic as an OBJECT library instead of a static one" OFF)

if(LIBPIC_OBJECT_LIBRARY)
    add_library(libpic OBJECT ${LIBPIC_SOURCES})
else()
    add_library(libpic STATIC ${LIBPIC_SOURCES})
endif()

target_include_directories(libpic PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

option(LIBPIC_BUILD_TESTS "Build LibPIC unit tests" OFF)

if(LIBPIC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
