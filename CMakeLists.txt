cmake_minimum_required(VERSION 3.8) # for cxx_std_17 in CMAKE_CXX_KNOWN_FEATURES
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

# helper function
function(expect_str ret should_succeed)
  if(${should_succeed})
    set(${ret} "success" PARENT_SCOPE)
  else()
    set(${ret} "failure" PARENT_SCOPE)
  endif()
endfunction()

#helper function
function(std_num ret use_cxx17)
  if(${use_cxx17})
    set(${ret} 17 PARENT_SCOPE)
  else()
    set(${ret} 11 PARENT_SCOPE)
  endif()
endfunction()

#helper function
function(noexc_str ret require_noexcept)
  if(${require_noexcept})
    set(${ret} "reqnoexc" PARENT_SCOPE)
  else()
    set(${ret} "allowexc" PARENT_SCOPE)
  endif()
endfunction()

# setup a compilation test, choosing a source file, whether the compilation
# should succeed, whether to require C++17, whether to require noexcept, and
# providing a counter that identifies what parts of the code to activate
function(add_compilation_test src should_succeed cxx17 require_noexcept countid)
  # calculate components and names of executable, test, and define
  expect_str(expect ${should_succeed})
  noexc_str(noexc ${require_noexcept})
  std_num(stdn ${cxx17})
  set(stdstr cpp${stdn})
  set(stdftr cxx_std_${stdn})

  string(CONCAT exe "compilation_" ${expect} "_" ${stdstr} "_" ${noexc} "_"
         ${countid})
  string(CONCAT tst "test_" ${exe})
  string(CONCAT def "test_" ${noexc} ${countid})

  # create and configure underlying executable target
  add_executable(${exe} ${src})
  set_target_properties(${exe}
                        PROPERTIES EXCLUDE_FROM_ALL ON
                        EXCLUDE_FROM_DEFAULT_BUILD ON)
  target_compile_definitions(${exe} PRIVATE ${def})
  target_compile_features(${exe} PRIVATE ${stdftr})
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

# Add tests for compilations that are always expected to succeed. Note that the
# tests that are added on the first outer iteration (test_0) have no
# corresponding preprocessor check in the code and serve to test compilation
# when no test macro is explicitly defined.
foreach(count RANGE 0 3) # range is inclusive on both ends in cmake
  foreach(cxx17 FALSE TRUE)
    foreach(reqne FALSE TRUE)
      add_compilation_test(compile_time_noexcept_tests.cpp TRUE ${cxx17}
                           ${reqne} ${count})
    endforeach()
  endforeach()
endforeach()

# Add tests for compilations that are expected to fail when using
# SG_REQUIRE_NOEXCEPT_IN_CPP17 in C++17 and succeed otherwise
foreach(count RANGE 4 15) # range is inclusive on both ends in cmake
  foreach(cxx17 FALSE TRUE)
    foreach(reqne FALSE TRUE)
      set(success TRUE)
      if(cxx17 AND reqne)
        set(success FALSE) # compilation should fail only in this case
      endif()
      add_compilation_test(compile_time_noexcept_tests.cpp ${success} ${cxx17}
                           ${reqne} ${count})
    endforeach()
  endforeach()
endforeach()

add_custom_target(test_verbose COMMAND ${CMAKE_CTEST_COMMAND} --verbose)
enable_testing()
