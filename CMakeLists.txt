cmake_minimum_required(VERSION 3.8) # for CXX_STANDARD 17
project(scope_guard)

include(CMakeDependentOption)
include(CheckCXXCompilerFlag)

option(SG_CXX17 "option to compile using c++17 rather than c++11" OFF)

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
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic \
                                          -Wno-unused")
  check_cxx_compiler_flag("-Wnoexcept-type" HAS_NOEXCEPT_TYPE_WARNING)
  if(HAS_NOEXCEPT_TYPE_WARNING)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-noexcept-type")
  endif()
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
endif()

# setup compilation tests
# each test activates a specific preprocessor macro that can be used to select
# different sections of the code to compile
function(add_compilation_test src should_succeed count)
  # derive executable, test, and macro names
  if(${should_succeed})
    set(expect "success")
  else()
    set(expect "failure")
  endif()

  string(CONCAT exe "compilation_" ${expect} ${count})
  string(CONCAT tst "test_" ${exe})
  string(CONCAT def "test_" ${expect} ${count})

  # create and configure underlying executable target
  add_executable(${exe} ${src})
  set_target_properties(${exe}
                        PROPERTIES EXCLUDE_FROM_ALL ON
                        EXCLUDE_FROM_DEFAULT_BUILD ON)
  target_compile_definitions(${exe} PRIVATE ${def})
  target_compile_options(${exe} PRIVATE "-Wno-unused-variable") # TODO this assumes GCC!

  # create and configure test
  add_test(NAME ${tst}
           COMMAND ${CMAKE_COMMAND} --build . --target ${exe} --config $<CONFIG>
           WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

  if(${should_succeed})
    set_tests_properties(${tst} # confirm no warnings in successful test
                         PROPERTIES FAIL_REGULAR_EXPRESSION "warn;Warn;WARN")
  else()
    set_tests_properties(${tst} PROPERTIES WILL_FAIL TRUE) # expect a failure
  endif()

endfunction()

# add compile time tests
foreach(count_success RANGE 0 3) # ints in [0, 3]
  add_compilation_test(compile_time_tests.cpp TRUE ${count_success})
endforeach()

if(SG_CXX17 AND SG_REQUIRE_NOEXCEPT_IN_CPP17)
  foreach(count_failure RANGE 1 12) # ints in [1, 12]
    add_compilation_test(compile_time_tests.cpp FALSE ${count_failure})
  endforeach()
endif()

# find catch 
include(GNUInstallDirs)
find_package(Catch2 REQUIRED)

# setup catch tests
add_executable(catch_tests catch_tests.cpp)
add_test(NAME catch_tests
         COMMAND "catch_tests" "--order" "lex" "--use-colour" "yes")

add_custom_target(test_verbose COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

enable_testing()
