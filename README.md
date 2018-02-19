# scope_guard [under construction]
A C++11 scope guard.

## [Intended] features
- [x] &ge; C++11
- [ ] Exception safe
- [x] Simple (as simple as possible while maintaining safety)
- [x] General (accepts anything that can be bound to a
`std::function<void()noexcept>`)
- [x] Fast (no runtime `std::function` penalties)
- [x] No dependencies to use (besides C++11 compiler and standard library)
- [x] No implicitly ignored return - callback must return `void` (clients can
write a lambda to explicitly ignore it if they want)
- [ ] No exceptions - callback must be noexcept (clients have to explicitly
handle any exceptions)
- [x] `snake_case` style (except for template arguments - just like the
standard library)
- [x] Auxiliary `make` function to deduce template parameters in pre-C++17
- [x] No macros - just write explicit lambda or bind or what have you
- [ ] Tested
- [x] Unlicense(d)

## Usage
To use,  simply clone this repository, copy the header file within, and include 
it. There are no dependencies (besides a &ge; C++11 compiler).

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
    $ make test
    ```
