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
option(HOLDEM_BUILD_GUI "Build graphical version (raylib is downloaded automatically)" ON)

if(HOLDEM_BUILD_GUI)
    find_package(raylib 5.5 QUIET)
    if(NOT raylib_FOUND)
        include(FetchContent)
        if(POLICY CMP0135)
            cmake_policy(SET CMP0135 NEW)
        endif()
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
        message(STATUS "Завантаження raylib 5.5 (лише при першому налаштуванні)...")
        FetchContent_Declare(raylib
                GIT_REPOSITORY https://github.com/raysan5/raylib.git
                GIT_TAG 5.5
                GIT_SHALLOW TRUE
                GIT_PROGRESS TRUE)
        FetchContent_MakeAvailable(raylib)
    endif()
endif()

if(MSVC)
    add_compile_options(/W4 /utf-8 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow)
endif()

add_subdirectory(src/core)
add_subdirectory(src/eval)
add_subdirectory(src/game)
add_subdirectory(src/analysis)
add_subdirectory(src/ai)
add_subdirectory(src/ui)
if(HOLDEM_BUILD_GUI)
    add_subdirectory(src/gui)
endif()
add_subdirectory(app)

if(HOLDEM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
