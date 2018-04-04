<sup>_The key words "MUST", "MUST NOT", "REQUIRED", "SHALL", "SHALL
NOT", "SHOULD", "SHOULD NOT", "RECOMMENDED",  "MAY", and "OPTIONAL" in this
document are to be interpreted as described in RFC 2119._</sup>

## Client interface

The public interface consists of a template function to create scope guard
objects, a few members of those objects, and one boolean compilation option that
can be activated with a preprocessor macro definition.

Here is an outline of the client interface:

- [Maker function template](#maker-function-template)
- [Scope guard objects](#scope-guard-objects)
  * [Invariants](#invariants)
  * [List of available public members](#list-of-available-public-members)
  * [List of deleted public members](#list-of-deleted-public-members)
  * [Member type `calback_type`](#member-type-calback_type)
  * [Member function `dismiss`](#member-function-dismiss)
  * [Member move constructor](#member-move-constructor)
  * [Member destructor](#member-destructor)
- [Compilation option `SG_REQUIRE_NOEXCEPT_IN_CPP17`](#compilation-option-sg_require_noexcept_in_cpp17)

### Maker function template

The free function template `make_scope_guard` resides in `namespace sg` and
is the primary element of the public interface &ndash; most uses do not require
anything else. It provides a way for clients to create a scope guard object with
the specified callback as _associated callback_.

Scope guards execute their _associated callback_ when they are destroyed, unless
they were meanwhile [dismissed](#member-function-dismiss) or
[moved](#member-move-constructor). Like other variables, scope guards with
automatic storage are destroyed when they go out of scope, which explains their
name.

This function template is [SFINAE-friendly](#sfinae-friendliness).

###### Function signature:

```c++
  template<typename Callback>
  /* unspecified */ make_scope_guard(Callback&& callback)
  noexcept(std::is_nothrow_constructible<Callback, Callback&&>::value);
```

###### Preconditions:

The template and function arguments need to respect certain preconditions. They
are listed here and discussed in more detail ahead.

- [invocable with no arguments](#invocable-with-no-arguments)
- [void return](#void-return)
- [_nothrow_-invocable](#nothrow-invocable)
- [_nothrow_-destructible if non-reference](#nothrow-destructible-if-non-reference-template-argument)
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
collapsing). Notice this is always the case if `Callback` is a reference type.

###### Example:

```c++
const auto guard = sg::make_scope_guard([]() noexcept { /* do stuff */ });
```

### Scope guard objects

Scope guard objects have some unspecified type with:

#### Invariants:
1. A scope guard object that is in an _active state_ executes its
_associated callback_ exactly once when leaving scope.
2. A scope guard that is in _inactive state_ does not execute its
_associated callback_

#### List of available public members:

1. Type `callback_type`
2. `dismiss` function
3. move constructor
4. destructor

#### List of _deleted_ public members:

1. default constructor
2. copy constructor
3. copy assignment operator
4. move assignment operator

Note: Deleted special members cannot be used, but they participate in overload
resolution. They are explicitly disallowed and that is part of the client's
interface.

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
using my_cb = typename decltype(guard)::callback_type; /* where guard
                                              is a scope guard object */
```

#### Member function `dismiss`
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
{ // example with early returns
  if(!do_step1())
    return false;

  auto undo = sg::make_scope_guard(rollback);
  if(!do_step2());
    return false;
  if(!do_step3());
    return false;

  undo.dismiss(); // <-- using dismiss
  return true;
}
```

#### Member move constructor

Objects created with `make_scope_guard` can be moved. This
possibility exists mainly to allow initialization with assignment syntax, as in
`auto g1 = make_scope_guard(f);`. In general, it allows transferring
scope guarding responsibility: `auto g2 = std::move(g1);`.

A scope-guard move transfers the callback and corresponding call responsibility
(or lack thereof). Moved-from scope guards are valid but useless. They are best
regarded as garbage awaiting destruction.

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
  auto g1 = sg::make_scope_guard([]() noexcept { std::cout << "bla"; });
  {
    auto g2 = std::move(g1);
  } // g2 leaves scope here
  std::cout << "ble";
} // g1 leaves scope here
std::cout << "bli";
// prints "blablebli"
```

#### Member destructor

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
[_nothrow_-invocable](#nothrow-invocable) and
[_nothrow_-destructible if non-reference](#nothrow-destructible-if-non-reference-template-argument).

###### Example:

Non applicable.

### Compilation option `SG_REQUIRE_NOEXCEPT_IN_CPP17`

If &ge;C++17 is used, the preprocessor macro `SG_REQUIRE_NOEXCEPT_IN_CPP17`
can be defined to make `scope_guard`'s constructor require a nothrow invocable
at compile time.

Notice however, that this restricts the types that `scope_guard` accepts
considerably, as explained
[below](#implications-of-requiring-noexcept-callbacks-at-compile-time).
That is one of the reasons why it is disabled by default. The
other is to maintain the same behavior as in &lt;C++17.

This option has no effect unless &ge;C++17 is used.

###### Example:

```c++
#define SG_REQUIRE_NOEXCEPT_IN_CPP17 // (no effect in <C++17)
#include "scope_guard.hpp"
make_scope_guard([](){}); // ERROR: need noexcept (if >=C++17)
make_scope_guard([]() noexcept {}); // OK
```