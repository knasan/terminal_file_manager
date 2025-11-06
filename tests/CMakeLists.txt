include(FetchContent)

# Fetch GoogleTest
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)

# For Windows: Prevent overriding parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Include GoogleTest module
include(GoogleTest)

# Test executable
add_executable(tmf-lib_test
    test_filesafety.cpp
    test_fileinfo.cpp
    test_duplicatefinder.cpp
    test_filescanner.cpp
)

target_link_libraries(tmf-lib_test
    PRIVATE
    tmf-lib
    GTest::gtest_main
    GTest::gmock_main
)

# Discover tests
gtest_discover_tests(tmf-lib_test)
