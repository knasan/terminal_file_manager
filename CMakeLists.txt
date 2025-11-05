# Mindestanforderung an CMake festlegen
cmake_minimum_required(VERSION 4.1)

# Projektname definieren
project(DuplicateFileFinder CXX)

# C++ Standard festlegen (FTXUI bevorzugt C++17 oder neuer)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Aktiviere strenge Warnungen für alle Targets
#if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
#    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow)
#elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
#    add_compile_options(/W4 /permissive- /w44265 /w44062)
#endif()


# Library
add_subdirectory(lib)

# CLI application
add_subdirectory(cli)

# TUI application (optional)
option(BUILD_TUI "Build FTXUI version" ON)
if(BUILD_TUI)
    add_subdirectory(tui)
endif()

