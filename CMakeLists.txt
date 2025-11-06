# CMakeLists.txt

cmake_minimum_required(VERSION 4.1)

project(terminal_file_manager CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(BUILD_TESTS "Build unit tests" ON)

#if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
#    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow)
#elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
#    add_compile_options(/W4 /permissive- /w44265 /w44062)
#endif()


if(NOT CMAKE_BUILD_TYPE)
    # Setzt den Standard auf Debug, wenn kein Build-Typ über die Kommandozeile angegeben wird
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

# Library
add_subdirectory(lib)

# CLI application
add_subdirectory(cli)

# Tests (optional)
if(BUILD_TESTS)
    message(STATUS "Building tests")
    enable_testing()
    add_subdirectory(tests)
endif()

# TUI application (optional)
option(BUILD_TUI "Build FTXUI version" ON)
if(BUILD_TUI)
    add_subdirectory(tui)
endif()
