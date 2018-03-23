# scope_guard


A public, general, simple, fast, and SFINAE-friendly C++11 scope guard which
forbids implicitly ignored returns and optionally enforces `noexcept` at compile
time (in C++17).

_The key words "MUST", "MUST NOT", "REQUIRED", "SHALL", "SHALL
NOT", "SHOULD", "SHOULD NOT", "RECOMMENDED",  "MAY", and "OPTIONAL" in this
document are to be interpreted as described in RFC 2119._

## Outline

- [scope_guard](#scope_guard)
  * [Outline](#outline)
  * [Introduction](#introduction)
  * [Acknowledgments](#acknowledgments)
  * [Features](#features)
  * [Setup](#setup)
  * [Client interface](#client-interface)
  * [Preconditions](#preconditions)
  * [Design choices and concepts](#design-choices-and-concepts)
  * [Tests](#tests)

## Introduction

A scope guard is an object that employs RAII to guarantee execution of the
provided callback when leaving scope, be it through a _fall-through_, a return,
or an exception. That callback can be a a function, a function pointer, a
functor, a lambda, a bind result, a std::function, or a reference to any of
these, as long as it respects the preconditions.

All necessary code is provided in a [single header](scope_guard.hpp)
(the remaining code is for tests.)

Usage is simple:

```c++
#include "scope_guard.hpp"
...
{
  ...
  auto guard = make_scope_guard(my_callback);
    ...
} // my_callback is executed
```

## Acknowledgments

The concept of "scope guard" was [first proposed](http://drdobbs.com/184403758)
publicly by Andrei Alexandrescu and Petru Marginean and it is well known in the
C++ community these days. It has been proposed for standardization (see
[N4189](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4189.pdf))
but is still not part of the standard library, as of March 2018. While there
are many implementations available, I did not find any with the
characteristics I aim for here.

## Features

### Main features
- [x] &ge; C++11
- [x] Reduced interface
- [x] Fast (no added runtime `std::function` penalties)
- [x] General: accepts any callable that respects a few
[preconditions](#preconditions)
- [x] no implicitly ignored return (see [below](#void-return))
- [x] Option to enforce `noexcept` in C++17
(see [below](#compilation-option-sg_require_noexcept_in_cpp17))
- [x] _SFINAE-friendliness_ (see [below](#sfinae-friendliness))
- [x] expose correct exception specification (conditional `noexcept`,
see [below](#conditional-noexcept))

### Other characteristics
- [x] No dependencies to use (besides &ge;C++11 compiler and standard library)
- [x] No macros to make guard &ndash; just write explicit lambda or bind or
what have you
- [x] Extensively tested, with both
[compile time tests](compile_time_tests.cpp) and
[runtime-tests](catch_tests.cpp)
- [x] Carefully documented (and adheres to
[RFC2119](https://tools.ietf.org/html/rfc2119))
- [x] Unlicense'd
- [x] `snake_case` style

### Issues

Bug reports and suggestions are welcome. If you find that something is incorrect
or could be improved, feel free to open an issue.

## Setup

Setup consists merely of making the [header file](scope_guard.hpp) available to
the compiler. That can be achieved by any of the following options:

1. placing it directly in the client project
2. placing it in a central include path that is known to the compiler
3. placing it in an arbitrary path and configuring the compiler to include that
path

The preprocessor definition `SG_REQUIRE_NOEXCEPT_IN_CPP17` MAY be provided
to the compiler. The effect of this option is explained
[below](#compilation-option-sg_require_noexcept_in_cpp17).

## Client interface

The public interface consists of a template function to create scope guard
objects, a few members of those objects, and one compilation option (through a
preprocessor macro definition.)

Here is an outline of the client interface:

- [Maker function template](#maker-function-template)
- [Scope guard objects](#scope-guard-objects)
  * [Invariants:](#invariants)
  * [Member type `calback_type`](#member-type-calback_type)
  * [Dismiss](#dismiss)
  * [Move constructor](#move-constructor)
  * [Destructor](#destructor)
  * [Compilation option `SG_REQUIRE_NOEXCEPT_IN_CPP17`](#compilation-option-sg_require_noexcept_in_cpp17)

### Maker function template

The free function template `make_scope_guard` is the primary public interface
&ndash; most uses do not require anything else. It provides a way for clients to
create a scope guard object from a specified callback. Scope guards created this
way are automatically destroyed when going out of scope, at which point they
execute their _associated_ callback, unless they were meanwhile
_[dismissed](#dismiss)_ or _[moved](#move-constructor)_.

This function template is [SFINAE-friendly](#sfinae-friendliness).

###### Function signature:

```c++
  template<typename Callback>
  /* unspecified */ make_scope_guard(Callback&& callback)
  noexcept(std::is_nothrow_constructible<Callback, Callback&&>::value);
```

###### Preconditions:

The template and function arguments need to respect certain preconditions. They
are listed here and discussed in more detail [below](#preconditions).

- [invocable with no arguments](#invocable-with-no-arguments)
- [void return](#void-return)
- [_nothrow_-invocable](#_nothrow_-invocable)
- [_nothrow_-destructible if non-reference](#_nothrow_-destructible-if-non-reference-template-argument)
template argument
- [const-invocable if const reference](#const-invocable-if-const-reference)
- [appropriate lifetime if lvalue reference](#appropriate-lifetime-if-lvalue-reference)
- [movable or copyable if non-reference](#movable-or-copyable-if-non-reference)

###### Postconditions:

A scope guard object is returned with

- the provided callback as _associated callback_
- _active_ state

###### Exception specification:

As the signature shows, instances of this function template are `noexcept` _iff_
`Callback` can be _nothrow_ constructed from `Callback&&` (after reference
collapse). Notice this is always the case if `Callback` is a reference type.

###### Example:

```c++
const auto guard = make_scope_guard([]() noexcept { /* do something */ });
```

### Scope guard objects

Scope guard objects have some unspecified type with:

#### Invariants:
1. A scope guard object that is in an _active state_ executes its
_associated callback_ exactly once when leaving scope.
2. A scope guard that is in _inactive state_ never executes its
_associated callback_

###### Public members:

1. Type `callback_type`
2. `dismiss` function
3. move constructor
4. destructor

###### Public _deleted_ members:

Scope guards cannot be default-constructed, copy-constructed, or assigned to.

1. default constructor
2. copy constructor
3. copy assignment operator
4. move assignment operator

Note: Deleted special members cannot be used, but they participate in overload
resolution. In other words, it is part of the client's interface that they are
explicitly disallowed.

#### Member type `calback_type`

Template argument deduction allows the underlying callback type to be
automatically derived from the function argument. That type &ndash; `Callback`
in the signature of `make_scope_guard` [above](#maker-function-template) &ndash;
is provided as the member type `callback_type`.

###### Member type declaration:

```c++
typedef Callback callback_type;
```

###### Example:

```c++
`decltype(guard)::callback_type` // where guard is a scope guard object
```

#### Dismiss
Scope guards can be dismissed to cancel callback execution. Dismissed scope
guards are valid but useless. They are best regarded as garbage awaiting
destruction.

###### Member function signature:

```c++
void dismiss() noexcept;
```

###### Preconditions:
None.

###### Postconditions:
The dismissed scope guard is in _inactive state_.

###### Exception specification:

`noexcept`.

###### Example:

```c++
bool do_transaction()
{
  do_part1();
  auto undo = make_scope_guard(undo_part1);
  do_part2();
  undo.dismiss();
}
```

#### Move constructor

Objects created with `make_scope_guard` can be moved. This
possibility exists mainly to allow initialization with assignment syntax, as in
`auto g1 = make_scope_guard(f);`. It may also be useful for explicit _ownership_
transfer: `auto g2 = std::move(g1);`

A scope-guard move transfers the callback and call responsibility (or lack
thereof). Moved-from scope guards are valid but useless. They are best regarded
as garbage awaiting destruction.

###### Member function signature:

Unspecified.

###### Preconditions:

None.

###### Postconditions:

- The moved-to guard is in _active state_ if the moved-from guard was in
_active state_ before the move operation.
- The moved-to guard is in _inactive state_ if the moved-from guard was in
_inactive state_ before the move operation.
- The moved-to guard is associated with the callback that was associated with
the moved-from guard before the move operation;
- The moved-from guard is in _inactive state_.
- The moved-from guard is associated with an unspecified callback.

###### Exception specification:

`noexcept` _iff_ `Callback` can be _nothrow_ constructed from `Callback&&`
(after reference collapse). Notice this is always the case if `Callback` is a
reference type.

###### Example:
```c++
{
  auto g1 = make_scope_guard([]() noexcept { std::cout << "bla"; });
  {
    auto g2 = std::move(g1);
  } // g2 leaves scope here
  std::cout << "ble";
} // g1 leaves scope here
std::cout << "bli";
// prints "blablebli"
```

#### Destructor

Scope guards have a destructor.

###### Member function signature:

Unspecified.

###### Preconditions:

None.

###### Postconditions:

- the callback was executed _iff_ the guard was in _active state_ before
destruction;
- the guard is no longer valid

###### Exception specification:

`noexcept`. This motivates two of the preconditions discussed below:
[_nothrow_-invocable](#_nothrow_-invocable) and
[_nothrow_-destructible if non-reference](#_nothrow_-destructible-if-non-reference-template-argument).

###### Example:

Non applicable.

### Compilation option `SG_REQUIRE_NOEXCEPT_IN_CPP17`

If &ge;C++17 is used, the preprocessor macro `SG_REQUIRE_NOEXCEPT_IN_CPP17`
can be defined to make `scope_guard`'s constructor require a nothrow invocable
at compile time.

Notice however, that this restricts the types that `scope_guard` accepts
considerably. That is one of the reasons why it is disabled by default. The
other is to maintain the same behavior as in &lt;C++17. Please consider some
further implications
[below](#implications-of-requiring-noexcept-callbacks-at-compile-time).

This option has no effect unless &ge;C++17 is used.

###### Example:

```c++
#define SG_REQUIRE_NOEXCEPT_IN_CPP17 // (no effect in <C++17)
#include "scope_guard.hpp"
make_scope_guard([](){}); // ERROR: need noexcept (if >=C++17)
make_scope_guard([]() noexcept {}); // OK
```

## Preconditions

This section explains the preconditions that the callback passed to
`make_scope_guard` is subject to. Here they are listed again:

- [invocable with no arguments](#invocable-with-no-arguments)
- [void return](#void-return)
- [_nothrow_-invocable](#_nothrow_-invocable)
- [_nothrow_-destructible if non-reference](#_nothrow_-destructible-if-non-reference-template-argument)
template argument
- [const-invocable if const reference](#const-invocable-if-const-reference)
template argument
- [appropriate lifetime if lvalue reference](#appropriate-lifetime-if-lvalue-reference)
template argument
- [movable or copyable if non-reference](#movable-or-copyable-if-non-reference)
template argument

#### invocable with no arguments

The callback MUST be invocable with no arguments. The client MAY use a capturing
lambda to easily pass something that takes arguments in its original form.

###### Compile time enforcement:

This precondition _is enforced_ at compile time.

###### Example:

```c++
void my_resource_release(Resource& r) noexcept;
make_scope_guard(my_resource_release); // ERROR: which resource?
make_scope_guard([&some_resource]() noexcept
                 { my_resource_release(some_resource); }); // OK
```

#### void return

The callback MUST return void. Returning anything else is
[intentionally](TODOlink) rejected. The user MAY wrap their call in a
lambda that ignores the return.

###### Compile time enforcement:

This precondition _is enforced_ at compile time.

###### Example:

```c++
bool foo() noexcept;
make_scope_guard(foo); // ERROR: does not return void
make_scope_guard([]() noexcept {/*bool ignored =*/ foo();}); // OK
```

#### _nothrow_-invocable

The callback SHOULD NOT throw _when invoked_. Clients SHOULD pass only
`noexcept` callbacks to `make_scope_guard`. Throwing from a callback that is
associated with an active scope guard when it goes out of scope results in a
call to `std::terminate`. Clients MAY use a lambda to wrap something that throws
in a `try-catch` block, choosing to deal with or ignore exceptions.

###### Compile time enforcement:

_By default_, this precondition _is not enforced_ at compile time. That can be
[changed]((#option-sg_require_noexcept_in_cpp17)) in &ge;C++17.

###### Example:

```c++
bool throwing() { throw std::runtime_error{"attention"}; }
make_scope_guard([]() noexcept
{
  try { throwing(); } catch(...) { /* taking the blue pill */ }
});
```

#### _nothrow_-destructible if non-reference template argument

If the template argument `Callback` is not a reference, then it MUST NOT throw
upon destruction. The user MAY use a reference if necessary:

###### Compile time enforcement:

This precondition _is enforced_ at compile time.

###### Example:

```c++
struct throwing
{
  ~throwing() noexcept(false) { throw std::runtime_error{"some error"}; }
  void operator()() noexcept {}
};

try
{
  throwing tmp;
  make_scope_guard([&tmp](){ t(); })
} catch(...) { /* handle somehow */ }
```

#### const-invocable if const reference

If the callback is `const`, there MUST be an appropriate `const` `operator()`
that respects the other preconditions.

###### Compile time enforcement:

This precondition _is enforced_ at compile time.

###### Example:

```c++
  struct Foo
  {
    Foo() {} // need user provided ctor
    void operator()() const noexcept { }
  } const foo;

  auto guard = make_scope_guard(foo); // OK, foo const with const op()
```

#### appropriate lifetime if lvalue reference

If the template argument is an lvalue reference, then the function argument MUST
be valid at least until the corresponding scope guard goes out of scope. Notice
this is the case when the template argument is deduced from both lvalues and
lvalue references.

###### Compile time enforcement:

This precondition _is not enforced_ at compile time.

#### movable or copyable if non-reference

If the template argument is not a reference, then it MUST be
either copyable or movable (or both). This is the case when the template
argument is deduced from rvalues and rvalue references.

###### Compile time enforcement:

This precondition _is enforced_ at compile time.

## Design choices and concepts

This section tries to clarify concepts used in the interface and discusses the
rationale for some design decisions. The outline is:

- [SFINAE friendliness](#sfinae-friendliness)
- [No extra arguments](#no-extra-arguments)
- [Conditional `noexcept`](#conditional-noexcept)
- [Private constructor](#private-constructor)
- [no return](#no-return)
- [nothrow invocation](#nothrow-invocation)
- [Implications of requiring `noexcept` callbacks at compile time](#implications-of-requiring-noexcept-callbacks-at-compile-time)

### SFINAE friendliness

The function `make_scope_guard` is _SFINAE-friendly_. In other words, when the
compiler tries to deduce a template argument, an invalid application of
`make_scope_guard` that is caused by a failure to substitute a candidate type
(e.g. because the argument is not callable) does not cause a compilation error
if any other substitution is still possible.

You can look for "sfinae" in [catch tests](catch_tests.cpp) for examples.

### No extra arguments

As the signature shows, `make_scope_guard` accepts no arguments beyond
the callback (see the [related precondition](#invocable-with-no-arguments)).
I could not see a compelling need for them. When lambdas and binds are
available, the scope guard is better off keeping a _single responsibility_ of
guarding the scope and leaving the responsibility of closure to those other
types.

### Conditional `noexcept`

The exception specification of `make_scope_guard` is such that making a scope
guard is often a _nothrow_ operation. Notice in particular that
`make_scope_guard` is `noexcept` in the following cases:

1. when `callback` is an lvalue or lvalue reference (`Callback` deduced to be
a reference)
2. when `callback` is an rvalue or rvalue reference of a type with:
    * a `noexcept` move constructor, and
    * a `noexcept` destructor (anyway required when `Callback` is not a
    reference &ndash; see [preconditions](#preconditions))

However, `make_scope_guard` is _not_ `noexcept` when it needs to rely upon an
operation that is not `noexcept` (e.g. rvalue with `noexcept(false)` move
constructor)

You can look for `noexcept` in [compilation tests](compile_time_tests.cpp) for
examples.

### Private constructor

The form of construction that is used by `make_scope_guard` to create scope
guards is not part of the public interface. The purpose is to prevent
unintentional misuse, preventing dynamic storage duration (_scope_ guards would
need a different name if that was allowed) and guiding the user to type
deduction with universal references, which would not be available in the
constructor (at least in &lt;C++17.)

### no return

This forces the client to confirm their intention, by explicitly
writing code to ignore a return, if that really is what they want. The idea is
not only to catch unintentional cases but also to highlight intentional ones for
code readers.

### nothrow invocation

Throwing from a callback implies throwing from scope guards' destructor, causing
the program to terminate. This follows the same approach as custom deleters in
such standard library types as `unique_ptr` and `shared_ptr` (see
`[unique.ptr.single.ctor]` and `[unique.ptr.single.dtor]` in the C++
standard.)

I considered making scope guards' destructor conditionally `noexcept` instead,
but that is not advisable either and could create a false sense of safety
(better _fail-fast_-ish, I suppose).

### Implications of requiring `noexcept` callbacks at compile time

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

Therefore, the additional safety sacrifices generality. Of course, clients can
still use functions and lambdas to wrap anything else, e.g.:

    make_scope_guard([&foo]()noexcept{std::bind(bar, foo)})

Personally, I favor using this option if possible, but it requires C++17 as the
exception specification is not part of a function's type
[until then](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0012r1.html).

## Tests

A number of compile time and run-time tests can be automatically run in multiple
configurations.

There are a few dependencies to execute the tests:
- C++11 capable compiler, preferably C++17 capable (c++1z is fine if it provides
the symbol
[__cpp_noexcept_function_type](http://en.cppreference.com/w/cpp/experimental/feature_test))
- A version of [Cmake](https://cmake.org/) (at least version 3.8) in the
interval [3.8, 4)
- A version of [Catch2](https://github.com/catchorg/Catch2)

### Instructions for running the tests
(For GNU/Linux, should be analogous in other systems.)

1. Install [cmake](https://cmake.org/) &ndash; a version in the interval
[3.8, 4);
2. Get and install [Catch2](https://github.com/catchorg/Catch2):
    ```sh
    $ git clone https://github.com/catchorg/Catch2 <catch_src_dir>
    $ mkdir <catch_bin_dir>
    $ cd <catch_bin_dir>
    $ cmake <catch_src_dir>
    $ make
    $ make install
    ```
3. Clone and build this repo:
    ```sh
    $ git clone https://github.com/ricab/scope_guard.git <guard_src_dir>
    $ mkdir <guard_bin_dir>
    $ cd <guard_bin_dir>
    $ cmake [options] <guard_src_dir>
    $ make
    $ make test
    ```

To speed things up, the last two commands can be given a number of threads to
execute in parallel. For instance:

```sh
make -j4 # only 4 separate compilations are done at this step
make test ARGS=-j16 # I find N*2 for N hardware threads to be a good choice here (considering I/O overhead)
```

This will run catch and compile time tests with different combinations of
SG_REQUIRE_NOEXCEPT_IN_CPP17 and C++ standard, depending on compiler
capabilities. If the compiler supports exception specifications as part of the
type system (
[P0012R1](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0012r1.html)
),
both C++11 and C++17 cases are tested (cases X, Y, W, and Z in the table below).
Otherwise, only C++11 is tested (cases X and Y below). Notice that `noexcept` is
only effectively required in case Z.

| standard/pp-define                                   | c++11 | c++17  |
| ---------------------------------------------------- |:-----:|:------:|
| **SG_REQUIRE_NOEXCEPT_IN_CPP17 undefined**           | X     |   W    |
| **SG_REQUIRE_NOEXCEPT_IN_CPP17 defined**             | Y     |  *Z*   |

Note: to obtain more output (e.g. because there was a failure), the command
`make test` can be replaced with `VERBOSE=1 make test_verbose`. This shows the
command lines used in compilation tests, as well as the test output.
