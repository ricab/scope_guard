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
  * [Overview of public members](#overview-of-public-members)
  * [List of deleted public members](#list-of-deleted-public-members)
  * [Member type `callback_type`](#member-type-callback_type)
  * [Member function `dismiss`](#member-function-dismiss)
  * [Member move constructor](#member-move-constructor)
  * [Member destructor](#member-destructor)
- [Compilation option `SG_REQUIRE_NOEXCEPT_IN_CPP17`](#compilation-option-sg_require_noexcept_in_cpp17)

### Maker function template

The free function template `make_scope_guard` resides in `namespace sg` and
is the primary element of the public interface &ndash; most uses do not require
anything else. It provides a way for clients to create a scope guard object that
is _associated_ with the specified callback.

Scope guards execute their _associated_ callback when they are destroyed, unless
they were meanwhile [dismissed](#member-function-dismiss) or
[moved](#member-move-constructor). Like other variables, scope guards with
automatic storage are destroyed when they go out of scope, hence the name.

This function template is [SFINAE-friendly](design.md#sfinae-friendliness).

###### Function signature:

```c++
  template<typename Callback>
  /* unspecified return type */ make_scope_guard(Callback&& callback)
  noexcept(std::is_nothrow_constructible<Callback, Callback&&>::value);
```

###### Preconditions:

The template and function arguments need to respect certain preconditions. They
should all be intuitive to C++ programmers, with the possible exception of
precondition 2. They are summarized here and discussed in more detail
[elsewhere](precond.md).

1. [invocable with no arguments](precond.md#invocable-with-no-arguments)
2. [void return](precond.md#void-return)
3. [_nothrow_-invocable](precond.md#nothrow-invocable)
4. [_nothrow_-destructible if non-reference](precond.md#nothrow-destructible-if-non-reference-template-argument)
template argument
5. [const-invocable if const reference](precond.md#const-invocable-if-const-reference)
6. [appropriate lifetime if lvalue reference](precond.md#appropriate-lifetime-if-lvalue-reference)
7. [movable or copyable if non-reference](precond.md#movable-or-copyable-if-non-reference)

By default, precondition 3 is not enforced at compile time. Precondition 6 is
not enforced at compile time. All other preconditions are enforced at compile
time.

###### Postconditions:

A scope guard object is returned with

- the provided callback as _associated_ callback
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

Scope guard objects have some unspecified type that MUST NOT be used as a base
class.

#### Invariants:
1. A scope guard object that is in an _active_ state executes its
_associated_ callback exactly once when leaving scope.
2. A scope guard that is in _inactive_ state does not execute its
_associated_ callback

#### Overview of public members:

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
resolution. They are listed here because they are explicitly disallowed and
that can be considered as part of the the client's interface.

#### Member type `callback_type`

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
The dismissed scope guard is in _inactive_ state.

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

Standard move constructor.

###### Preconditions:

None.

###### Postconditions:

- The moved-from guard is in _inactive_ state.
- The moved-from guard is _associated_ with an unspecified callback.
- The moved-to guard is in the same _activity_ state as the moved-from guard was
in before the move operation (_active_ moved-to _iff_ previously _active_
moved-from).
- The moved-to guard is _associated_ with the callback that was associated with
the moved-from guard before the move operation;

###### Exception specification:

`noexcept` _iff_ `Callback` can be _nothrow-constructed_ from `Callback&&`
(after reference collapse). Notice this is always the case when `Callback` is a
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

Scope guards have a (non-virtual) destructor.

###### Member function signature:

Standard destructor.

###### Preconditions:

None.

###### Postconditions:

- the callback was executed _iff_ the guard was in _active_ state before
destruction;
- the guard is no longer valid

###### Exception specification:

`noexcept`. This motivates two of the preconditions:
[_nothrow_-invocable](precond.md#nothrow-invocable) and
[_nothrow_-destructible if non-reference](precond.md#nothrow-destructible-if-non-reference-template-argument).

###### Example:

Non applicable.

### Compilation option `SG_REQUIRE_NOEXCEPT_IN_CPP17`

If &ge;C++17 is used, the preprocessor macro `SG_REQUIRE_NOEXCEPT_IN_CPP17`
can be defined to require _nothrow-invocable_ callbacks in `make_scope_guard`
at compile time.

Notice however, that this restricts accepted callback types
[considerably](design.md#implications-of-requiring-noexcept-callbacks-at-compile-time).

This option is disabled by default and enabling it has no effect unless
&ge;C++17 is used.

###### Example:

```c++
#define SG_REQUIRE_NOEXCEPT_IN_CPP17 // (no effect in <C++17)
#include "scope_guard.hpp"
make_scope_guard([](){}); // ERROR: need noexcept (if >=C++17)
make_scope_guard([]() noexcept {}); // OK
```
