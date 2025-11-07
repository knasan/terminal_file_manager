include(FetchContent)

FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v6.1.9
)

FetchContent_MakeAvailable(ftxui)

add_executable(tfm 
    main.cpp
    filemanagerui.cpp
)

target_include_directories(tfm
    PUBLIC ${CMAKE_SOURCE_DIR}/utils
)


target_link_libraries(tfm PRIVATE 
    tmf-lib
    ftxui::screen
    ftxui::dom
    ftxui::component
    pthread
)
