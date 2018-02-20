# scope_guard [under construction]
A C++11 scope guard.

## Main features
- [x] &ge; C++11
- [ ] Single `make_scope_guard` function interface
- [x] Fast (no runtime `std::function` penalties)
- [x] General: accepts any callable (that respects the restrictions below)
- [x] No implicitly ignored return - callback must return `void` (clients can
write a lambda to explicitly ignore it if they want)
- [ ] Exception safe in C++17 (callback must be noexcept)
- [ ] dtor conditionally noexcept in &lt;C++17
- [x] No dependencies to use (besides &ge;C++11 compiler and standard library)

### Other characteristics
- [x] `snake_case` style
- [x] No macros - just write explicit lambda or bind or what have you
- [ ] Extensively tested
- [x] Unlicense(d)

## Usage
To use,  simply clone this repository, copy the header file within, and include 
it - there are no dependencies (besides a &ge;C++11 compiler). Then do something
like:

    auto guard = make_scope_guard([]()noexcept{ std::cout << "bye scope\n"; });

The tests in catch_tests.cpp have many code examples.

## Running tests
There are two dependencies to execute the tests:
    * Cmake (at least version 3.1)
    * Catch2
    
#### Instructions (for GNU/Linux, should be analogous in other systems):
1. Install [cmake](https://cmake.org/) (&ge; v3.1)
2. Get and install [Catch2](https://github.com/catchorg/Catch2):
    ```
    $ git clone https://github.com/catchorg/Catch2 <catch_src_dir>
    $ mkdir <catch_bin_dir>
    $ cd <catch_bin_dir>
    $ cmake <catch_src_dir>
    $ make
    $ make install
    ```
3. Clone and build this repo:
    ```
    $ git clone https://github.com/ricab/scope_guard.git <guard_src_dir>
    $ mkdir <guard_bin_dir>
    $ cd <guard_bin_dir>
    $ cmake <guard_src_dir>
    $ make
    $ make tests
    ```
