# scope_guard
## Table of contents

- [scope_guard](#scope-guard)
  * [Table of contents](#table-of-contents)
  * [Intro](#intro)
  * [Features](#features)
    + [Main features](#main-features)
    + [Other characteristics](#other-characteristics)
  * [Usage](#usage)
    + [Preconditions](#preconditions)
      - [no arguments](#no-arguments)
      - [void return](#void-return)
      - [no throw](#no-throw)
    + [Option `SG_REQUIRE_NOEXCEPT_IN_CPP17`](#option--sg-require-noexcept-in-cpp17-)
      - [Implications of requiring `noexcept` callbacks at compile time](#implications-of-requiring--noexcept--callbacks-at-compile-time)
  * [Tests](#tests)
    + [Instructions for running the tests](#instructions-for-running-the-tests)

<sup><sub>><i><a href='http://ecotrust-canada.github.io/markdown-toc/'>Table of contents generated with markdown-toc</a></i></sub></sup>


## Intro

A public, general, simple, fast, and tested C++11 scope_guard which forbids
implicitly ignored returns and optionally enforces noexcept at compile time in
C++17. Usage is easy:

```c++
auto guard = make_scope_guard(my_callback);
```

A scope guard is an object that employs RAII to guarantee execution of the
provided callback when leaving scope, be it through a fallthrough, a return,
or an exception.

All necessary code is provided in a [single header](scope_guard.hpp)
(the remaining code is for tests.)

## Features

### Main features
- [x] &ge; C++11
- [x] Interface consists _mostly_ of a single function (`make_scope_guard`)
- [x] Fast (no added runtime `std::function` penalties)
- [x] General: accepts any callable that respects the preconditions
[below](#preconditions)
- [x] no implicitly ignored return (see [below](#void-return))
- [x] Option to enforce `noexcept` in C++17
(see [below](#option-sg_require_noexcept_in_cpp17))

### Other characteristics
- [x] No dependencies to use (besides &ge;C++11 compiler and standard library)
- [x] No macros to make guard - just write explicit lambda or bind or what have
you
- [x] Extensively tested, with both [compile-time tests](compile_time_noexcept_tests.cpp) and [runtime-tests](catch_tests.cpp)
- [x] Unlicense(d)
- [x] `snake_case` style

## Usage
To use, simply copy the [header file](scope_guard.hpp) to your project (or
somewhere accessible to your compiler), and include it - there are no
dependencies (besides a &ge;C++11 compiler). Then do something like:

```c++
auto guard1 = make_scope_guard(my_callback);
auto guard2 = make_scope_guard([]()noexcept{ /* do something */ });
...
```

The template function `make_scope_guard` has the following signature.

```c++
template<typename Callback>
scope_guard make_scope_guard(Callback&& callback) noexcept;
```

The `scope_guard` type provides the deduced `Callback` type as a nested type.
It can be accessed with `decltype(sgobj)::callback_type;`, assuming `sgobj`
is the result of a successful `make_scope_guard` call.

See [tests](catch_tests.cpp) for use-case examples.

### Preconditions

The callback that is used to create a `scope_guard` must respect the following preconditions.

#### no arguments
The callback must be invocable with no arguments. Use a lambda to pass
something that takes arguments in its original form. For example:

```c++
void my_resource_release(Resource& r) noexcept;
make_scope_guard(my_resource_release); // ERROR: which resource?
make_scope_guard([&some_resource]() noexcept { my_resource_release(some_resource); }); // OK
```

This precondition is enforced at compile time.

#### void return

The callback must return void. Returning anything else is intentionally
forbidden. This forces the client to confirm their intention, by explicitly
writing code to ignore a return, if that really is what they want. For example:

```c++
bool foo() noexcept;
make_scope_guard(foo); // ERROR: does not return void
make_scope_guard([]() noexcept {/*bool ignored =*/ foo();}); // OK
```

The idea is not only to catch unintentional cases but also to highlight
intentional ones for code readers.

This precondition is enforced at compile time.

#### no throw

The callback _is required_ not to throw. If you want to use something that
might throw, you can wrap it in a `try-catch` block, explicitly choosing what
to do with any exceptions that might arise. For example:

```c++
bool throwing() { throw std::runtime_error{"some error"}; }
make_scope_guard([]() noexcept
{
  try { throwing(); } catch(...) { /* choosing to ignore */ }
});
```

Notice that, **_by default_, this precondition is not enforced at compile
time**. To enforce it at compile time
[read on](#option--sg-require-noexcept-in-cpp17-).

Throwing from a `scope_guard` callback results in a call to
`std::terminate`. This follows the same approach as custom deleters in such
standard library types as `unique_ptr` and `shared_ptr` (see
`[unique.ptr.single.ctor]` and `[unique.ptr.single.dtor]` in the C++
standard.)

I acknowledge that the destructor for `scope_guard` could be conditionally
`noexcept` instead, but that is not advisable either and could
create a false sense of safety (better _fail-fast_-ish, I suppose).

Personally, I favor enforcing that the callback be
`noexcept` at compile-time, but this not only restricts acceptable callback
types, to my knowledge it is not even possible until C++17. That is because the
exception specification is not part of the type system
[until then](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0012r1.html).

### Option `SG_REQUIRE_NOEXCEPT_IN_CPP17`

If &ge;C++17 is used, the preprocessor macro `SG_REQUIRE_NOEXCEPT_IN_CPP17`
can be defined to make `scope_guard`'s constructor require a nothrow invocable
at compile-time, e.g.

```c++
#define SG_REQUIRE_NOEXCEPT_IN_CPP17 // (no effect in <C++17)
#include "scope_guard.hpp"
make_scope_guard([](){}); // ERROR: need noexcept
make_scope_guard([]() noexcept {}); // OK
```

Notice however, that this restricts the types that `scope_guard` accepts
considerably. That is one of the reasons why it is disabled by default. The
other is to maintain the same behavior as in &lt;C++17.

This option has no effect unless &ge;C++17 is used.

#### Implications of requiring `noexcept` callbacks at compile time

Unfortunately, even in C++17 things are not ideal, and information on
exception specification is not propagated to types like `std::function` or
the result of `std::bind` and `std::ref`. For instance, the following code
does not compile in C++17:

```c++
void f() noexcept { }
auto stdf_noexc = std::function<void(&)()noexcept>{f}; // ERROR (at least in g++ and clang++)
auto stdf_declt = std::function<decltype(f)>{f};       // ERROR (at least in g++ and clang++)
auto stdf = std::function<void()>{f};                  // fine, but drops noexcept info
```

Since `SG_REQUIRE_NOEXCEPT_IN_CPP17` means rejecting anything that
is not known to possess an `operator() noexcept`, the additional
safety sacrifices generality. Of course the user can still use functions and
lambdas to wrap anything else, e.g.:

    make_scope_guard([&foo]()noexcept{std::bind(bar, foo)})

## Tests
A number of compile-time and run-time tests can be automatically run in multiple
configurations.

There are a few dependencies to execute the tests:
- C++11 capable compiler, preferably C++17 capable (c++1z is fine if it provides
the symbol [__cpp_noexcept_function_type](http://en.cppreference.com/w/cpp/experimental/feature_test))
- [Cmake](https://cmake.org/) (at least version 3.8)
- [Catch2](https://github.com/catchorg/Catch2)

### Instructions for running the tests
(For GNU/Linux, should be analogous in other systems.)

1. Install [cmake](https://cmake.org/) (&ge; v3.8)
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
    $ cmake [options] <guard_src_dir>
    $ make
    $ make test
    ```

This will run catch and compile-time tests with different combinations of
SG_REQUIRE_NOEXCEPT_IN_CPP17 and C++ standard, depending on compiler
capabilities. If the compiler supports exception specifications as part of the
type system ([P0012R1](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0012r1.html)),
both C++11 and C++17 cases are tested (cases X, Y, W, and Z in the table below).
Otherwise, only C++11 is tested (cases X and Y below). Notice that noexcept is
only effectively required in case Z.

| standard/pp-define                                   | c++11 | c++17  |
| ---------------------------------------------------- |:-----:|:------:|
| **SG_REQUIRE_NOEXCEPT_IN_CPP17 undefined**           | X     |   W    |
| **SG_REQUIRE_NOEXCEPT_IN_CPP17 defined**             | Y     |  *Z*   |

Note: to obtain more output (e.g. because there was a failure), run
`VERBOSE=1 make test_verbose` instead, to get the command lines used in
compilation tests, as well as the test output.
