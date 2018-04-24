<sup>_The key words "MUST", "MUST NOT", "REQUIRED", "SHALL", "SHALL
NOT", "SHOULD", "SHOULD NOT", "RECOMMENDED",  "MAY", and "OPTIONAL" in this
document are to be interpreted as described in RFC 2119._</sup>

## Preconditions in detail

This section explains the preconditions that the callback passed to
`make_scope_guard` is subject to. They are hopefully all intuitive, with the
possible exception of [void return](#void-return).

- [invocable with no arguments](#invocable-with-no-arguments)
- [void return](#void-return)
- [_nothrow_-invocable](#nothrow-invocable)
- [_nothrow_-destructible if non-reference](#nothrow-destructible-if-non-reference-template-argument)
template argument
- [const-invocable if const reference](#const-invocable-if-const-reference)
template argument
- [appropriate lifetime if lvalue reference](#appropriate-lifetime-if-lvalue-reference)
template argument
- [movable or copyable if non-reference](#movable-or-copyable-if-non-reference)
template argument

### invocable with no arguments

The callback MUST be invocable with no arguments. Additional arguments are
[intentionally](design.md#no-extra-arguments) not supported. The client MAY use
a capturing lambda to easily pass something that takes arguments in its original
form.

###### Compile time enforcement:

This precondition _is enforced_ at compile time.

###### Example:

```c++
void my_release(Resource& r) noexcept;
sg::make_scope_guard(my_release); // ERROR: which resource?
sg::make_scope_guard(my_release, my_resource); // ERROR: 1 arg only, please
sg::make_scope_guard([&my_resource]() noexcept
                     { my_release(my_resource); }); // OK
```

### void return

The callback MUST return void. Returning anything else is
[intentionally](design.md#no-return) rejected. The user MAY wrap their call in a
lambda that ignores the return value.

###### Compile time enforcement:

This precondition _is enforced_ at compile time.

###### Example:

```c++
bool foo() noexcept;
sg::make_scope_guard(foo); // ERROR: does not return void
sg::make_scope_guard([]() noexcept {/*bool ignored =*/ foo();}); // OK
```

### _nothrow_-invocable

The callback SHOULD NOT throw _when invoked_. Marking callbacks `noexcept` is
RECOMMENDED. Throwing from a callback that is _associated_ with an _active_
scope guard when it goes out of scope results in a call to `std::terminate`.
Clients MAY use a lambda to wrap something that throws in a `try-catch` block,
choosing to deal with or ignore exceptions.

###### Compile time enforcement:

_By default_, this precondition _is not enforced_ at compile time. That can be
[changed](interface.md#compilation-option-sg_require_noexcept_in_cpp17) when
using &ge;C++17.

###### Example:

```c++
bool throwing() { throw std::runtime_error{"attention"}; }
sg::make_scope_guard([]() noexcept {
  try { throwing(); } catch(...) { /* taking the blue pill */ }
});
```

### _nothrow_-destructible if non-reference template argument

If the template argument `Callback` is not a reference, then the callback
MUST NOT throw upon destruction. The user MAY use a reference if necessary:

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
  throwing_dtor tmp;
  sg::make_scope_guard([&tmp](){ tmp(); })
  // guard destroyed, tmp still alive
} // tmp only destroyed here
catch(...) { /* handle somehow */ }
```

### const-invocable if const reference

If the callback is `const`, it MUST be _const-invocable_. In practice, that
means that an appropriate `const` `operator()` respecting the other
preconditions MUST exist.

###### Compile time enforcement:

This precondition _is enforced_ at compile time.

###### Example:

```c++
  struct Foo
  {
    Foo() {} // (need user provided ctor)
    void operator()() const noexcept { }
  } const foo;

  auto guard = sg::make_scope_guard(foo); // OK, foo const with const op()
```

### appropriate lifetime if lvalue reference

If the template argument is an lvalue reference, then the function argument MUST
be valid at least until it is not _associated_ with any active scope guard.
Notice this is the case when the template argument is deduced from both lvalues
and lvalue references.

###### Compile time enforcement:

This precondition _is not enforced_ at compile time.

### movable or copyable if non-reference

If the template argument is not a reference, then it MUST be
either copyable or movable (or both). This is the case when the template
argument is deduced from rvalues and rvalue references.

###### Compile time enforcement:

This precondition _is enforced_ at compile time.