## Tests

A number of compile-time and run-time tests can be automatically run in multiple
configurations.

There are a few dependencies to execute the tests:
- C++11 capable compiler, preferably C++17 capable (c++1z is fine if it provides
the symbol
[__cpp_noexcept_function_type](http://en.cppreference.com/w/cpp/experimental/feature_test))
- [Cmake](https://cmake.org/) &ndash; a version in the interval [3.8, 4)
- [Catch2](https://github.com/catchorg/Catch2) &ndash; any sub-version of v2 (now obtained automatically via CMake's FetchContent module)

### Instructions for running the tests
(For GNU/Linux, should be analogous in other systems.)

1. Install cmake
2. Clone and build this repo:
    ```sh
    $ git clone https://github.com/ricab/scope_guard.git <guard_src_dir>
    $ mkdir <guard_bin_dir>
    $ cd <guard_bin_dir>
    $ cmake <guard_src_dir>
    $ make
    $ make test
    ```

To speed things up, the last two commands can be given a number of threads to
execute in parallel. For instance:

```sh
make -j4 # only 4 compilations needed
make test ARGS=-j16 # On an otherwise idle machine, I suggest 2x the
                    # number of HW threads (to compensate for I/O)
```

This will run catch and compile-time tests with different combinations of
`SG_REQUIRE_NOEXCEPT_IN_CPP17` and C++ standard, depending on compiler
capabilities. If the compiler supports exception specifications as part of the
type system
([P0012R1](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0012r1.html)),
both C++11 and C++17 standards are tested (cases X, Y, W, and Z in the table
below). Otherwise, only C++11 is tested (cases X and Y below). Notice that
`noexcept` is only effectively required in case Z.

| pp-define\standard                                   | c++11 | c++17  |
| ---------------------------------------------------- |:-----:|:------:|
| **SG_REQUIRE_NOEXCEPT_IN_CPP17 undefined**           | X     |   W    |
| **SG_REQUIRE_NOEXCEPT_IN_CPP17 defined**             | Y     |  *Z*   |

Note: to obtain more output (e.g. because there was a failure), the command
`make test` can be replaced with `VERBOSE=1 make test_verbose`. This shows the
command lines used in compilation tests, as well as detailed test output.
