# scope_guard
## Table of contents


- [scope_guard](#scope-guard)
  * [Table of contents](#table-of-contents)
  * [Intro](#intro)
  * [Features](#features)
    + [Main features](#main-features)
    + [Other characteristics](#other-characteristics)
  * [Usage](#usage)
    + [Signature](#signature)
  * [Detailed documentation](#detailed-documentation)
    + [Type deduction and SFINAE](#type-deduction-and-sfinae)
    + [Conditional `noexcept`](#conditional--noexcept-)
    + [Remaining interface](#remaining-interface)
      - [Why so little?](#why-so-little-)
    + [Moving scope guards](#moving-scope-guards)
    + [Invariants](#invariants)
    + [Preconditions](#preconditions)
      - [invocable with no arguments](#invocable-with-no-arguments)
      - [void return](#void-return)
      - [no throw](#no-throw)
    + [Option `SG_REQUIRE_NOEXCEPT_IN_CPP17`](#option--sg-require-noexcept-in-cpp17-)
      - [Implications of requiring `noexcept` callbacks at compile time](#implications-of-requiring--noexcept--callbacks-at-compile-time)
  * [Tests](#tests)
    + [Instructions for running the tests](#instructions-for-running-the-tests)

<sup><sub>><i><a href='http://ecotrust-canada.github.io/markdown-toc/'>Table of contents generated with markdown-toc</a></i></sub></sup>


## Intro

A public, general, simple, fast, and SFINAE-friendly C++11 scope guard which
forbids implicitly ignored returns and optionally enforces noexcept at compile
time (in C++17).

Usage is easy:

```c++
auto guard = make_scope_guard(my_callback);
```

A scope guard is an object that employs RAII to guarantee execution of the
provided callback when leaving scope, be it through a fall-through, a return,
or an exception.

All necessary code is provided in a [single header](scope_guard.hpp)
(the remaining code is for tests.)

## Features

### Main features
- [x] &ge; C++11
- [x] Interface consists _mostly_ of a single function (`make_scope_guard`)
- [x] Fast (no added runtime `std::function` penalties)
- [x] General: accepts any callable that respects a few [preconditions](#preconditions)
- [x] no implicitly ignored return (see [below](#void-return))
- [x] Option to enforce `noexcept` in C++17
(see [below](#option-sg_require_noexcept_in_cpp17))
- [x] _SFINAE-friendliness_ (see [below](#type-deduction-and-sfinae))
- [x] expose correct exception specification (conditionally-`noexcept` maker,
see [below](#conditional--noexcept-))

### Other characteristics
- [x] No dependencies to use (besides &ge;C++11 compiler and standard library)
- [x] No macros to make guard &ndash; just write explicit lambda or bind or
what have you
- [x] Extensively tested, with both
[compile-time tests](compile_time_tests.cpp) and
[runtime-tests](catch_tests.cpp)
- [x] Unlicense(d)
- [x] `snake_case` style

## Usage
To use, simply copy the [header file](scope_guard.hpp) to your project (or
somewhere accessible to your compiler), and include it. Then do something
like:

```c++
auto guard1 = make_scope_guard(my_callback);
auto guard2 = make_scope_guard([]()noexcept{ /* do something */ });
...
```

See [tests](catch_tests.cpp) for use-case examples.

### Signature

The template function `make_scope_guard` has the following signature.

```c++
  template<typename Callback>
  /* unspecified */ make_scope_guard(Callback&& callback)
  noexcept(std::is_nothrow_constructible<Callback, Callback&&>::value);
```

## Detailed documentation

While usage is meant to be mostly intuitive, documentation going into more
detail can be found below, along with the rationale for some of the design
decisions. I hope they make sense, but my opinions are not set in stone. I
welcome bug reports and improvement suggestions.

### Type deduction and SFINAE

Template argument deduction allows the underlying callback type to be
automatically derived from the argument. That type, which is identified as
`Callback` in the template signature above, is then provided as a nested
type with the name `callback_type` (e.g. `decltype(guard)::callback_type;`.)

The function `make_scope_guard` is _SFINAE-friendly_. In other words, an
invalid application of `make_scope_guard` by the compiler when trying to
deduce a template argument (e.g. because the argument is not callable) does
not cause a compilation error if any other substitution is still possible.

See [tests](catch_tests.cpp) for use-case examples.

### Conditional `noexcept`

The exception specification of `make_scope_guard` is such that making a scope
guard is often a _nothrow_ operation. Notice in particular that
`make_scope_guard` is `noexcept` in the following cases:

- when `callback` is an lvalue or lvalue reference
- when `callback` is an rvalue or rvalue reference of a type with:
    * a `noexcept` move constructor, and
    * a `noexcept` destructor

However, `make_scope_guard` is _not_ `noexcept` when it needs to rely upon an
operation that is not `noexcept` (e.g. rvalue with `noexcept(false)` destructor)

See [tests](compile_time_tests.cpp) for examples.

### Remaining interface

The resulting scope guard type provides `callback_type` as described
[above](#type-deduction-and-sfinae). Besides that, it provides only three public
member functions:

1. a destructor
2. a constructor taking a callback
3. a move constructor

The first and second items are not meant for direct usage &ndash; for obvious
reasons in the case of the destructor and because `make_scope_guard` is
preferable than a direct constructor call (at least until class template
argument deduction). 3. has relatively limited use but
is required for initialization with assignment syntax (see
[below](#moving-scope-guards) for details).

All remaining special member functions are `deleted`. In particular, scope
guards cannot be default-constructed, copy-constructed, or assigned to.

#### Why so little?

The goal of this approach is two-fold:

- to keep complexity to a minimum, avoiding superfluous object states leaking
into the domain of whatever problem the client is trying to solve.
- encouraging a single explicit way of doing something makes for clearer code

For instance, what problem could a default-constructed scope guard possibly
help solve? I cannot think of any real world example. Scope guards that do
nothing can always be created with an explicit no-op callback.

This is also why no "dismiss" operation is provided here, even though it is
often present in other implementations. In most cases, the scope of the guard
should be reduced; in the rest, the logic can be moved to the callback itself.

All of this makes for a relatively simple and general invariant. A scope guard
object in the code is almost always something that the reader can assume will
do something at the end of the scope and at no other occasion. There is only one
exception, caused by moves, whose benefits were judged to surpass these
downsides.

### Moving scope guards

Objects created with `make_scope_guard` can be moved. This
possibility exists mainly to allow initialization with assignment syntax and
auto type deduction, as in `auto foo = make_scope_guard(bar);`. Even though
there is no assignment taking place, this still requires the move constructor.
But having one means that, given an existing scope guard `g1`, the following
code is valid and useful for ownership transfer:

```c++
auto g2 = std::move(g1);
```

However, moved-from scope guards should be regarded with caution. They
constitute the single case violating the intended ideal that scope guards always
execute their callback (that responsibility is transfered to the scope guard
that is thus created). Alas, moved-from objects stay behind in C++ and we still
need them. In the case of scope guards they do not do anything special, even
though they are _technically valid_. The [invariants](#invariants) have to be
more complex because of them.

Here is an example of the effect of moving:

```c++
{
  auto g1 = make_scope_guard([]() noexcept { std::cout << "bla"; });
  {
    auto g2 = std::move(g1);
  } // g2 leaves scope here
  std::cout << "ble";
} // g1 leaves scope here
std::cout << "bli";
```

This would print `blablebli`, showing that `bla` is printed only once, when `g2`
leaves scope.

The state of moved-from scope guard exists for purely technical reasons only and
there is nothing useful that can be done with such objects. They are best
left alone and thought-of as garbage awaiting removal.

### Invariants

1. Objects created with `make_scope_guard` execute the provided `callback`
exactly once when leaving scope, except if they are successfully moved-from, in
which case they execute nothing.
2. Objects created by move-constructing an existing scope guard that was not
moved-from are considered to have been provided with the `callback` that the
source of the move had been provided with.
3. Objects created by move-constructing an existing scope guard that was already
moved-from are considered moved-from themselves.

### Preconditions

The callback that is used to create a scope guard must respect the following
preconditions.

#### invocable with no arguments
The callback must be invocable with no arguments. Use a lambda to pass
something that takes arguments in its original form. For example:

```c++
void my_resource_release(Resource& r) noexcept;
make_scope_guard(my_resource_release); // ERROR: which resource?
make_scope_guard([&some_resource]() noexcept
                 { my_resource_release(some_resource); }); // OK
```

This precondition is enforced at compile time.

#### void return

The callback must return void. Returning anything else is intentionally
rejected. This forces the client to confirm their intention, by explicitly
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
time**. To find out how to enforce it at compile time
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
type system (
[P0012R1](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0012r1.html)
),
both C++11 and C++17 cases are tested (cases X, Y, W, and Z in the table below).
Otherwise, only C++11 is tested (cases X and Y below). Notice that noexcept is
only effectively required in case Z.

| standard/pp-define                                   | c++11 | c++17  |
| ---------------------------------------------------- |:-----:|:------:|
| **SG_REQUIRE_NOEXCEPT_IN_CPP17 undefined**           | X     |   W    |
| **SG_REQUIRE_NOEXCEPT_IN_CPP17 defined**             | Y     |  *Z*   |

Note: to obtain more output (e.g. because there was a failure), run
`VERBOSE=1 make test_verbose` instead. This shows the command lines used in
compilation tests, as well as the test output.
