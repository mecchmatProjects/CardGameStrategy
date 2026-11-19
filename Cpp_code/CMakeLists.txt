cmake_minimum_required(VERSION 3.16)

project(TexasHoldem
        VERSION 1.0.0
        DESCRIPTION "Texas Hold'em simulator with optimal strategy analysis"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(HOLDEM_BUILD_TESTS "Build unit tests" ON)

if(MSVC)
    add_compile_options(/W4 /utf-8 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow)
endif()

add_subdirectory(src/core)

if(HOLDEM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
