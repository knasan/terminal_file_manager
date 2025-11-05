include(FetchContent)

FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v5.0.0
)

FetchContent_MakeAvailable(ftxui)

add_executable(tfm 
    main.cpp
    filemanagerui.cpp
)

target_link_libraries(tfm PRIVATE 
    fileinfo_lib        # <-- fileprocessor.cpp
    ftxui::screen
    ftxui::dom
    ftxui::component
)