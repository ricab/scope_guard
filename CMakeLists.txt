cmake_minimum_required(VERSION 3.8) # for CXX_STANDARD 17
project(scope_guard)

include(CheckCXXCompilerFlag)

# global compiler configurations
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# global compiler warnings
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

# find catch
include(GNUInstallDirs)
find_package(Catch2 REQUIRED)

# setup catch tests
add_executable(catch_tests catch_tests.cpp)
target_compile_features(catch_tests PRIVATE cxx_std_11) # TODO other options
add_test(NAME catch_tests
         COMMAND "catch_tests" "--order" "lex" "--use-colour" "yes")

# setup a compilation test, choosing a source file, whether the compilation
# should succeed, whether to require C++17, whether to require noexcept, and
# providing a counter that identifies what parts of the code to activate
function(add_compilation_test src should_succeed cxx17 require_noexcept countid)
  # calculate components and names of executable, test, and define
  if(${should_succeed})
    set(expect "success")
  else()
    set(expect "failure")
  endif()

  if(${cxx17})
    set(std "cxx_std_17")
  else()
    set(std "cxx_std_11")
  endif()

  if(${require_noexcept})
    set(except "reqnoexc")
  else()
    set(except "allowexc")
  endif()

  string(CONCAT exe
         "compilation_" ${expect} "_" ${std} "_" ${except} "_" ${countid})
  string(CONCAT tst "test_" ${exe})
  string(CONCAT def "test_" ${expect} ${countid})

  # create and configure underlying executable target
  add_executable(${exe} ${src})
  set_target_properties(${exe}
                        PROPERTIES EXCLUDE_FROM_ALL ON
                        EXCLUDE_FROM_DEFAULT_BUILD ON)
  target_compile_definitions(${exe} PRIVATE ${def})
  target_compile_features(${exe} PRIVATE ${std})
  if(${require_noexcept})
    target_compile_definitions(${exe} PRIVATE SG_REQUIRE_NOEXCEPT_IN_CPP17)
  endif()

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

# add pertinent versions of compile time tests, in what concerns c++ standard
# and whether to require noexcept
function(add_pertinent_compilation_tests src should_succeed countid)
  add_compilation_test(${src} ${should_succeed} FALSE FALSE ${countid})
  add_compilation_test(${src} ${should_succeed} TRUE FALSE ${countid})
  add_compilation_test(${src} ${should_succeed} TRUE TRUE ${countid})
endfunction()

# success tests, the first one (test_success0) with no corresponding pp check
# in the code, in order to test everything that is not disabled
foreach(sid RANGE 0 3) # ints in [0, 3]
  add_pertinent_compilation_tests(compile_time_noexcept_tests.cpp TRUE ${sid})
endforeach()

# failure tests
foreach(fid RANGE 1 12) # ints in [1, 12]
  add_pertinent_compilation_tests(compile_time_noexcept_tests.cpp FALSE ${fid})
endforeach()

add_custom_target(test_verbose COMMAND ${CMAKE_CTEST_COMMAND} --verbose)
enable_testing()
