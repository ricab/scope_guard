# scope_guard [under construction]
A C++11 scope guard.

## Main features
- [x] &ge; C++11
- [ ] Single `make_scope_guard` function interface
- [x] Fast (no runtime `std::function` penalties)
- [x] General: accepts any callable (that respects the restrictions below)
- [x] No implicitly ignored return - callback must return `void`; clients can
write a lambda to explicitly ignore it if they want
- [ ] Requires non-throwing callback (just like e.g. custom deleters in `unique_ptr` and `shared_ptr`);
- [ ] Option to enforce `noexcept` in C++17 (see [below](#considerations-on-noexcept))
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

```c++
auto guard = make_scope_guard([]()noexcept{ std::cout << "bye scope\n"; });
```

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

## Considerations on `noexcept`

Much as I would like to enforce at compile-time that the callback be
`noexcept`, to my knowledge that is not possible until C++17. This is because
the exception specification is not part of the type system
[until then](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0012r1.html).

So, even though the callback _is_ required not to throw, by default this is not
checked or protected against. `noexcept(false)` callbacks will still be
accepted by the compiler by default, causing undefined behavior if they do
throw. This is the same approach that the standard library takes e.g. with
`unique_ptr` (search for `[unique.ptr.single.ctor]` and `[unique.ptr.single.dtor]` in
the C++ standard.)

#### Option `SG_REQUIRE_NOEXCEPT_IN_CPP17`

If &ge;C++17 is used, the preprocessor macro `SG_REQUIRE_NOEXCEPT_IN_CPP17`
can be defined to reject any callable that is not
[nothrow invocable](http://en.cppreference.com/w/cpp/types/is_invocable).
Unfortunately, even in C++17 things are far from ideal, and information on
exception specification is not propagated to types like `std::function` or
the result of `std::bind`. For instance, the following code does not compile
in C++17:

```c++
void f() noexcept { }
auto stdf_noexc = std::function<void(&)()noexcept>{f}; // Error (at least in g++ and clang++)
auto stdf_declt = std::function<decltype(f)>{f};       // Error (at least in g++ and clang++)
auto stdf = std::function<void()>{f};                  // ok, but drops noexcept info
```

Since `SG_REQUIRE_NOEXCEPT_IN_CPP17` means rejecting anything that
is not known to possess an `operator()` that is `noexcept`, the additional
safety sacrifices generality. The user can still use lambdas to wrap anything
else, e.g.:

    make_scope_guard([&foo](){std::bind(bar, foo)})
