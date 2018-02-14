cmake_minimum_required(VERSION 3.1)

project(scope_guard)

set(CMAKE_CXX_STANDARD 11)

# find catch 
include(GNUInstallDirs)
find_package(Catch2 REQUIRED)

add_executable(scope_guard_test catch_tests.cpp)
add_test(NAME ScopeGuardTest COMMAND scope_guard_test)
enable_testing()
