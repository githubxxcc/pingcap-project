cmake_minimum_required(VERSION 3.17)
project(pingcap_interview)

set(CMAKE_CXX_STANDARD 17)
set(SOURCES
        buffer.h finder.h finder.cpp buffer.cpp
        )

add_executable(pingcap_interview main.cpp ${SOURCES})

#############################################
# Unit tests
# Add an library for the example classes
add_library(finder_lib
        ${SOURCES})
add_subdirectory(3rd_party/google-test)

# enable CTest testing
enable_testing()

# Add a testing executable
add_executable(unit_tests unit_tests.cpp)

target_link_libraries(unit_tests
        finder_lib
        GTest::GTest
        GTest::Main
        )

add_test(test_all unit_tests)