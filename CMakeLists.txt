cmake_minimum_required(VERSION 3.1)

project(scope_guard)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything -pedantic -Wno-c++98-compat")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
endif()

# find catch 
include(GNUInstallDirs)
find_package(Catch2 REQUIRED)

add_executable(scope_guard_test catch_tests.cpp)
add_test(NAME ScopeGuardTest COMMAND scope_guard_test)
enable_testing()
