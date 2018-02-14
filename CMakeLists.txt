cmake_minimum_required(VERSION 3.1)

project(scope_guard)

set(CMAKE_CXX_STANDARD 11)

# find catch 
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
find_package(Catch REQUIRED)

include_directories(${CATCH_INCLUDE_DIRS})

add_executable(scope_guard_test catch_tests.cpp)
add_test(NAME ScopeGuardTest COMMAND scope_guard_test)
enable_testing()
