# Mindestanforderung an CMake festlegen
cmake_minimum_required(VERSION 4.1)

# Projektname definieren
project(TUI_File_Manager CXX)

# C++ Standard festlegen (FTXUI bevorzugt C++17 oder neuer)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------------------------------------------------------------------
# 1. FTXUI von GitHub einbinden (FetchContent)
# ----------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG        main
)

FetchContent_MakeAvailable(ftxui)

# ----------------------------------------------------------------------
# 2. Ihr eigenes Programm definieren
# ----------------------------------------------------------------------
# Quellendateien Ihres Projekts definieren
add_executable(tfm 
    main.cpp
    fileprocessor.cpp
) 

# Die ausführbare Datei mit der FTXUI-Bibliothek verlinken
target_link_libraries(tfm PUBLIC 
    ftxui::dom 
    ftxui::screen 
    ftxui::component
)