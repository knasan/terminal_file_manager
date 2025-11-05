# cli/CMakeLists.txt

add_executable(tmf-cli 
    main.cpp
)

target_link_libraries(tmf-cli PRIVATE 
    tmf-lib
)