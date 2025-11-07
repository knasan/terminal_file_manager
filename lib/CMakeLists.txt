# lib/CMakeLists.txt
add_library(tmf-lib STATIC
    fileinfo/filescanner.cpp
    fileinfo/filesafety.cpp 
    fileinfo/fileprocessoradapter.cpp
)

target_include_directories(tmf-lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/fileinfo
)

target_compile_features(tmf-lib PUBLIC cxx_std_17)

# Optional: Header-Files for IDE ONLY
#target_sources(tmf-lib PUBLIC
#    FILE_SET HEADERS
#    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/fileinfo
#    FILES
#        fileinfo/fileinfo.hpp
#        fileinfo/ihashcalculator.hpp
#        fileinfo/fnv1a.hpp
#        fileinfo/filescanner.hpp
#        fileinfo/fileprocessor.hpp
#        fileinfo/duplicatefinder.hpp
#)