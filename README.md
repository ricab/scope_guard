# scope_guard


A public, general, simple, fast, and SFINAE-friendly C++11 scope guard which
forbids implicitly ignored returns and optionally enforces `noexcept` at compile
time (in C++17).

Usage is simple:

```c++
auto guard = make_scope_guard(my_callback);
```

A scope guard is an object that employs RAII to guarantee execution of the
provided callback when leaving scope, be it through a fall-through, a return,
or an exception.

All necessary code is provided in a [single header](scope_guard.hpp)
(the remaining code is for tests.)

## Table of contents

TODO


## Features

### Main features
- [x] &ge; C++11
- [x] Reduced interface
- [x] Fast (no added runtime `std::function` penalties)
- [x] General: accepts any callable that respects a few
[preconditions](#preconditions)
- [x] no implicitly ignored return (see [below](#void-return))
- [x] Option to enforce `noexcept` in C++17
(see [below](#option-sg_require_noexcept_in_cpp17))
- [x] _SFINAE-friendliness_ (see [below](#type-deduction-and-sfinae))
- [x] expose correct exception specification (conditional `noexcept`,
see [below](#conditional-noexcept))

### Other characteristics
- [x] No dependencies to use (besides &ge;C++11 compiler and standard library)
- [x] No macros to make guard &ndash; just write explicit lambda or bind or
what have you
- [x] Extensively tested, with both
[compile time tests](compile_time_tests.cpp) and
[runtime-tests](catch_tests.cpp)
- [x] Unlicense(d)
- [x] `snake_case` style

## Setup

Simply copy the [header file](scope_guard.hpp) to your project (or
somewhere accessible to your compiler), and include it. Then do something
like:

See [tests](catch_tests.cpp) for use-case examples.


## Client interface

The public interface consists of a template function to create scope guard
objects and a few members of those objects.

### Maker function template

The free function template `make_scope_guard` is the primary public interface
&ndash; most uses do not require anything else. It provides a way for clients to
create a scope guard object from a specified callback. Scope guards created this
way are automatically destroyed when going out of scope, at which point they
execute their _associated_ callback, unless they were meanwhile
_[dismissed](#dismiss)_ or _[moved](#move-constructor)_.

###### Function signature:

```c++
  template<typename Callback>
  /* unspecified */ make_scope_guard(Callback&& callback)
  noexcept(std::is_nothrow_constructible<Callback, Callback&&>::value);
```

###### Preconditions:

TODO: better list preconditions here in title form and link to details below
The template and function arguments need to respect certain preconditions,
which are documented [below](#preconditions).

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

### scope guard objects

Scope guard objects have some unspecified type with:

###### Invariants:
1. A scope guard object that is in an _active state_ executes its
_associated callback_ exactly once when leaving scope.
2. A scope guard that is in _inactive state_ never executes its
_associated callback_

###### Public members:

1. Type `callback_type`
2. `dismiss` function
3. move constructor
4. destructor

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
- The moved-from guard is associated with a valid but unspecified callback.

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
[no-throw-invocable](#no-throw-invocable) and
[no-throw-destructible if non-reference template argument](#no-throw-destructible-if-non-reference-template-argument).

###### Example:

Non applicable.

## Detailed documentation

While usage is meant to be mostly intuitive, more detailed documentation
can be found below, along with the rationale for some design
decisions. I hope they make sense, but I welcome bug reports and improvement
suggestions.

### No extra arguments

As the signature shows, `make_scope_guard` accepts no arguments beyond
the callback (see the [related precondition](#invocable-with-no-arguments)).
I could not see a need for them, but more on that [below](#why-so-little).

### Type deduction and SFINAE



The function `make_scope_guard` is _SFINAE-friendly_. In other words, an
invalid application of `make_scope_guard` by the compiler when trying to
deduce a template argument (e.g. because the argument is not callable) does
not cause a compilation error if any other substitution is still possible.

See [tests](catch_tests.cpp) for use-case examples.

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

See [tests](compile_time_tests.cpp) for examples.

### Remaining interface

The resulting scope guard type provides `callback_type` as described
[above](#type-deduction-and-sfinae). Besides that, it provides only three public
member functions:

1. a destructor
2. a constructor taking a callback
3. a move constructor

1 and 2 are not meant for direct usage &ndash; for obvious
reasons in the case of the destructor and because `make_scope_guard` is
preferable to a direct constructor call, as it supports type deduction,
allowing a _universal reference_ parameter. 3 is required for initialization
with assignment syntax (see [below](#moving-scope-guards) for details).

All remaining special member functions are `deleted`. In particular, scope
guards cannot be default-constructed, copy-constructed, or assigned to.

#### Why so little?

The goals of this approach are:

1. to keep complexity to a minimum, avoiding superfluous object states leaking
into the domain of whatever problem the client is trying to solve.
2. encouraging a single explicit way of doing something makes for clearer code
3. to strive for _single responsibility_

For instance, what problem could a default-constructed scope guard possibly
help solve? I cannot think of any real world example. Scope guards that do
nothing can always be created with an explicit no-op callback.

This is also why no "dismiss" operation is provided here, even though it is
often present in other implementations. The same is achievable by reducing the
scope of the guard or moving logic into the callback.

A decision following the principle in 3 is the exclusion of extra arguments.
Why add the responsibility of being a "closure" to the scope guard, when we have
lambdas and binds?

All of this makes for a relatively simple and general invariant. A scope guard
object in the code is _almost always_ something that the reader can assume will
do something at the end of the scope and at no other occasion. There is only one
exception, caused by moves, whose benefits were judged to surpass these
downsides.

### Moving scope guards

Objects created with `make_scope_guard` can be moved. This
possibility exists mainly to allow initialization with assignment syntax and
auto type deduction, as in `auto foo = make_scope_guard(bar);`.The following
code is also valid and useful for ownership transfer:

```c++
auto g2 = std::move(g1);
```

Moved-from scope guards should be regarded with some caution. If it
were not for them, scope guards would always execute their callback. With moves,
that responsibility is transfered and moved-from guards stay behind. They remain
_technically valid_, but there is nothing useful they can do. They exist for
purely technical reasons and are best left alone and thought-of as garbage
awaiting removal.

Here is an example of how it works:



This would print `blablebli`, showing that `bla` is printed only once, when `g2`
leaves scope.

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

#### Design choices

##### invocable with no arguments

The callback must be invocable with no arguments. Use a capturing lambda to
pass something that takes arguments in its original form. For example:

```c++
void my_resource_release(Resource& r) noexcept;
make_scope_guard(my_resource_release); // ERROR: which resource?
make_scope_guard([&some_resource]() noexcept
                 { my_resource_release(some_resource); }); // OK
```

_This precondition is enforced at compile time._

##### void return

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

_This precondition is enforced at compile time._

##### no-throw-invocable

The callback _is required_ not to throw when invoked. If you want to use
something that might throw, you can wrap it in a `try-catch` block, explicitly
choosing what to do with any exceptions that might arise. For example:

```c++
bool throwing() { throw std::runtime_error{"attention"}; }
make_scope_guard([]() noexcept
{
  try { throwing(); } catch(...) { /* choosing to ignore */ }
});
```

Throwing from a `scope_guard` callback results in a call to
`std::terminate`. This follows the same approach as custom deleters in such
standard library types as `unique_ptr` and `shared_ptr` (see
`[unique.ptr.single.ctor]` and `[unique.ptr.single.dtor]` in the C++
standard.) I considered making the destructor for `scope_guard` conditionally
`noexcept` instead, but that is not advisable either and could
create a false sense of safety (better _fail-fast_-ish, I suppose).

_By default, this precondition is not enforced at compile time_. To find out
how to change that, see [below](#option-sg_require_noexcept_in_cpp17).

#### More or less obvious

##### no-throw-destructible (if non-reference template argument)

If the template argument `Callback` is not a reference, then it must not throw
upon destruction. A reference can be used if necessary:

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

_This precondition is enforced at compile time._

##### const-invocable if const reference

If the callback is const, there must be an appropriate `const` `operator()`
that respects the other preconditions.

_This precondition is enforced at compile time._

##### appropriate lifetime if lvalue reference template argument

If the callback is passed by lvalue reference, it must be valid at least
until the corresponding scope guard goes out of scope. This is the case
when the template argument is deduced from an lvalue or lvalue reference.

_This precondition is not enforced_ at compile time._

##### movable or copyable if non-reference template argument

If the template argument `Callback` is not a reference, then it needs to be
either copyable or movable. This is the case when the template argument is
deduced from an rvalue or rvalue reference.

_This precondition is enforced at compile time._

### Option `SG_REQUIRE_NOEXCEPT_IN_CPP17`

If &ge;C++17 is used, the preprocessor macro `SG_REQUIRE_NOEXCEPT_IN_CPP17`
can be defined to make `scope_guard`'s constructor require a nothrow invocable
at compile time, e.g.

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

This will run catch and compile time tests with different combinations of
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
