cmake_minimum_required(VERSION 3.8) # for CXX_STANDARD 17

project(scope_guard)

option(SG_CXX17 "option to compile using c++17 rather than c++11" OFF)
include(CMakeDependentOption)

CMAKE_DEPENDENT_OPTION(SG_REQUIRE_NOEXCEPT_IN_CPP17
  "option to make scope_guard reject, at compile-time, anything that is not \
  guaranteed to be noexcept invocable (see project documentation)" OFF
  "SG_CXX17" OFF)

if(SG_CXX17)
  set(CMAKE_CXX_STANDARD 17)
else()
  set(CMAKE_CXX_STANDARD 11)
endif()

if(SG_REQUIRE_NOEXCEPT_IN_CPP17)
  add_definitions(-DSG_REQUIRE_NOEXCEPT_IN_CPP17)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything -pedantic \
                                          -Wno-c++98-compat \
                                          -Wno-unreachable-code \
                                          -Wno-padded")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wno-noexcept-type")
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

# setup an executable for catch tests
add_executable(catch_tests catch_tests.cpp)
add_test(NAME catch_tests
         COMMAND "catch_tests" "--order" "lex" "--use-colour" "yes")

add_custom_target(sgtests COMMAND ctest --verbose)

enable_testing()
