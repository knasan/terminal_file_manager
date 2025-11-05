# lib/CMakeLists.txt
add_library(fileinfo_lib STATIC
    fileinfo/filescanner.cpp
)

target_include_directories(fileinfo_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/fileinfo
)

target_compile_features(fileinfo_lib PUBLIC cxx_std_17)

# Optional: Header-Files for IDE ONLY
#target_sources(fileinfo_lib PUBLIC
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