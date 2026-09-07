cmake_minimum_required(VERSION 3.10)
project(LibPIC C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# if(NOT CMAKE_BUILD_TYPE)
#     set(CMAKE_BUILD_TYPE Debug)
# endif()

# add_compile_options(-Wall -Wextra)

add_library(libpic STATIC
    src/input.c
    src/input_group.c
    src/input_pattern.c
    src/output.c
    src/output_group.c
    src/timer.c
    src/trigger.c
)

target_include_directories(libpic PUBLIC include)

option(LIBPIC_BUILD_TESTS "Build LibPIC unit tests" ON)

if(LIBPIC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
