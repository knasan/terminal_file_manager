add_executable(dfinder_cli 
    main.cpp
)

target_link_libraries(dfinder_cli PRIVATE 
    fileinfo_lib
)