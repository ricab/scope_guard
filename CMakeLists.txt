cmake_minimum_required(VERSION 3.11) # for FetchContent
project(scope_guard)

# handle inclusions and dependencies
include(CheckCXXSymbolExists)
include(CheckCXXCompilerFlag)
include(GNUInstallDirs)

Include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v2.13.6
)

FetchContent_MakeAvailable(Catch2)

# global configurations
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# check for compiler support of noexcept in type
set(CMAKE_REQUIRED_FLAGS "-std=c++1z") # only for this check
CHECK_CXX_SYMBOL_EXISTS(__cpp_noexcept_function_type "" HAS_NOEXCEPT_IN_TYPE)
unset(CMAKE_REQUIRED_FLAGS)

# request C++11 explicitly by default - works around newer compilers otherwise
# omitting `-std=c++11` despite 11 being requested with target_compile_features
set(CMAKE_CXX_STANDARD 11)

# compiler warnings
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(clang_warnings "-Werror -Wall -Wextra -pedantic -Wno-c++98-compat \
    -Wno-unreachable-code -Wno-padded -Wno-exit-time-destructors \
    -Wno-global-constructors -Wno-unused-variable -Wno-unused-member-function \
    -Wno-unused-function -Wno-class-varargs")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${clang_warnings}")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wpedantic \
                                          -Wno-unused")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10)
    # earlier versions complain of unused "nodiscard" results in unevaluated contexts
    # (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=89070)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=unused-result")
  endif()

  check_cxx_compiler_flag("-Wnoexcept-type" HAS_NOEXCEPT_TYPE_WARNING)
  if(HAS_NOEXCEPT_TYPE_WARNING)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-noexcept-type")
  endif()

  option(ENABLE_COVERAGE "Enable coverage reporting" FALSE)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
endif()

# utility to derive a string with success or failure from boolean param
function(expect_str ret should_succeed)
  if(${should_succeed})
    set(${ret} "success" PARENT_SCOPE)
  else()
    set(${ret} "failure" PARENT_SCOPE)
  endif()
endfunction()

# utility to derive noexcept requirement status string from boolean param
function(noexc_str ret require_noexcept)
  if(${require_noexcept})
    set(${ret} "reqnoexc" PARENT_SCOPE)
  else()
    set(${ret} "allowexc" PARENT_SCOPE)
  endif()
endfunction()

# utility to derive standard number from boolean param
function(std_num ret use_cxx17)
  if(${use_cxx17})
    set(${ret} 17 PARENT_SCOPE)
  else()
    set(${ret} 11 PARENT_SCOPE)
  endif()
endfunction()

# utility to derive a string describing the c++ standard
function(std_str ret stdn)
  set(${ret} cpp${stdn} PARENT_SCOPE)
endfunction()

# utility to derive the c++ standard compile feature
function(std_ftr ret stdn)
  set(${ret} cxx_std_${stdn} PARENT_SCOPE)
endfunction()

# utility to derive strings for test configuration
function(derive_common_test_strings
         tst_ret exe_ret ftr_ret # out params
         base_name should_succeed cxx17 require_noexcept # in params
         )
  expect_str(expect ${should_succeed})
  noexc_str(noexc ${require_noexcept})
  std_num(stdn ${cxx17})
  std_str(std ${stdn})
  std_ftr(${ftr_ret} ${stdn})

  string(CONCAT ${exe_ret} ${base_name} "_" ${expect} "_" ${std} "_"
         ${noexc})
  string(CONCAT ${tst_ret} "test_" ${${exe_ret}})

  # return these
  set(${tst_ret} ${${tst_ret}} PARENT_SCOPE)
  set(${exe_ret} ${${exe_ret}} PARENT_SCOPE)
  set(${ftr_ret} ${${ftr_ret}} PARENT_SCOPE)
endfunction()

# utility to add executable and configure common properties
function(add_test_exe exe src ftr require_noexcept)
  add_executable(${exe} ${src})
  target_compile_features(${exe} PRIVATE ${ftr})
  if(${require_noexcept})
    target_compile_definitions(${exe} PRIVATE SG_REQUIRE_NOEXCEPT_IN_CPP17)
  endif()
endfunction()

# utility to add a batch of catch tests with the specified c++ standard and
# noexcept requirement
function(add_catch_tests_batch exe_ret src cxx17 require_noexcept)
  derive_common_test_strings(tst exe ftr # out params
      "catch_batch" TRUE ${cxx17} ${require_noexcept}) # in params
  add_test_exe(${exe} ${src} ${ftr} ${require_noexcept})
  target_link_libraries(${exe} PRIVATE Catch2::Catch2)

  add_test(NAME ${tst} COMMAND ${exe} "--order" "lex")

  set(${exe_ret} ${exe} PARENT_SCOPE) # return
endfunction()

# utility to add a compilation test, with the specified C++ standard and
# noexcept requirement, along with a success/failure expectation and a counter
# that identifies what parts of the code to activate
function(add_compilation_test src should_succeed cxx17 require_noexcept countid)
  derive_common_test_strings(tst exe ftr # out params
      "compilation" ${should_succeed} ${cxx17} ${require_noexcept}) # in params

  string(APPEND tst "_" ${countid})
  string(APPEND exe "_" ${countid})
  string(CONCAT def "test_" ${countid})
  add_test_exe(${exe} ${src} ${ftr} ${require_noexcept})

  # only build this when running tests (building _is_ the test)
  set_target_properties(${exe}
                        PROPERTIES EXCLUDE_FROM_ALL ON
                        EXCLUDE_FROM_DEFAULT_BUILD ON)

  # add macro to select active code section
  target_compile_definitions(${exe} PRIVATE ${def})

  # determine the test command
  set(tst_cmd ${CMAKE_COMMAND} --build . --target ${exe} --config $<CONFIG>)

  # if using MSBuild, prevent logo and warning summary
  if(${CMAKE_GENERATOR} MATCHES "[vV]isual [sS]tudio")
    list(APPEND tst_cmd -- /nologo /verbosity:minimal)
  endif()

  # create the test
  add_test(NAME ${tst}
           COMMAND ${tst_cmd}
           WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

  # configure the test to expect compilation success or failure
  if(${should_succeed})
    set_tests_properties(${tst} # confirm no warnings in successful test
                         PROPERTIES FAIL_REGULAR_EXPRESSION "warn;Warn;WARN")
  else()
    set_tests_properties(${tst} PROPERTIES WILL_FAIL TRUE) # expect a failure
  endif()
endfunction()

# utility to determine whether a compilation test should expect building to fail
function(expect_result ret count cxx17 reqne)
  if(${count} LESS 35)
    set(${ret} TRUE PARENT_SCOPE)

  elseif(${count} LESS 52)
    if(${cxx17} AND ${reqne})
      set(${ret} FALSE PARENT_SCOPE)
    else()
      set(${ret} TRUE PARENT_SCOPE)
    endif()
  else()
    set(${ret} FALSE PARENT_SCOPE)
  endif()
endfunction()

set(cxx17_possibilities FALSE)
if(HAS_NOEXCEPT_IN_TYPE)
  list(APPEND cxx17_possibilities TRUE)
endif()

# actually add the tests
foreach(cxx17 ${cxx17_possibilities})
  foreach(reqne FALSE TRUE)
    # add catch tests for this standard/noexcept-requirement combination
    add_catch_tests_batch(catch_batch_exe catch_tests.cpp ${cxx17} ${reqne})

    if(ENABLE_COVERAGE) # configure catch tests for coverage if needed
      target_compile_options(${catch_batch_exe} PRIVATE --coverage -O0)
      target_link_libraries(${catch_batch_exe} PRIVATE --coverage)
    else() # only run compile time tests in non-coverage builds
      # add noexcept compilation tests for this standard/noexcept-requirement
      # combination
      foreach(count RANGE 71) # range inclusive in cmake (so 0 or 72 tests)
                              # 0-34: always succeed (0 = all test macros off)
                              # 35-51: fail when requiring noexcept
                              # 52-71: always fail
        expect_result(success ${count} ${cxx17} ${reqne})
        add_compilation_test(compile_time_tests.cpp
                             ${success} ${cxx17} ${reqne} ${count})
      endforeach()
    endif()
  endforeach()
endforeach()

add_custom_target(test_verbose COMMAND ${CMAKE_CTEST_COMMAND} --verbose)
enable_testing()
