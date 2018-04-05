## Design choices and concepts

This section tries to clarify concepts used in the interface and discusses the
rationale for some design decisions. The outline is:

- [nothrow invocation](#nothrow-invocation)
- [Implications of requiring `noexcept` callbacks at compile time](#implications-of-requiring-noexcept-callbacks-at-compile-time)
- [no return](#no-return)
- [Conditional `noexcept`](#conditional-noexcept)
- [No extra arguments](#no-extra-arguments)
- [Private constructor](#private-constructor)
- [SFINAE friendliness](#sfinae-friendliness)

### Nothrow invocation

Scope guards rely on the destructor to execute the client's operation, but
throwing from destructors is bad practice in C++, so the client's operation
can't throw either. An
[option to enforce this](interface.md/#compilation-option-sg_require_noexcept_in_cpp17)
at compilation is available, but requires C++17 and has
[other implications](#implications-of-requiring-noexcept-callbacks-at-compile-time).

In either case, the destructor is declared noexcept. Consequently, throwing from
a scope guard's callback causes the program to terminate. This follows the same
approach as custom deleters in such standard library types as `unique_ptr` and
`shared_ptr` (see `[unique.ptr.single.ctor]` and `[unique.ptr.single.dtor]`
in the C++ standard).

I considered making scope guards' destructor conditionally `noexcept` instead,
but that is not advisable either and could create a false sense of safety
(better _fail-fast_ to discourage the client from relying or otherwise using
throwing callbacks).

### Implications of requiring `noexcept` callbacks at compile time

In C++17 the exception specification becomes part of a function's type. That
enables requiring `noexcept` callbacks for scope guards, something that was not
possible
[until then](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0012r1.html).

Unfortunately, even in C++17 things are not ideal, and information on
exception specification is not propagated to types like `std::function` or
the result of `std::bind` and `std::ref`. For instance, the following code
does not compile in C++17, at least in _gcc_ and _clang_:

```c++
void f() noexcept { }
auto stdf_noexc = std::function<void(&)()noexcept>{f}; // ERROR
auto stdf_declt = std::function<decltype(f)>{f};       // ERROR
auto stdf = std::function<void()>{f};                  /* fine, but drops
                                                          noexcept info */
```

Therefore, such types can't be used directly to create scope guards when
`noexcept` is
[enforced](interface.md/#compilation-option-sg_require_noexcept_in_cpp17). In
other words, that option's additional safety sacrifices generality. Of course,
clients can still use compliant alternatives to wrap anything else (e.g. a
`noexcept` lambda).

In order to maintain the same behavior accross C++ standard versions, this
option is disabled by default. I personally favor enabling it, if possible, in
&ge;C++17.

### No return

This forces the client to confirm their intention, by explicitly
writing code to ignore a return, if that really is what they want. The idea is
not only to catch unintentional cases but also to highlight intentional ones for
code readers.

### Conditional `noexcept`

The exception specification of `make_scope_guard` is such that making a scope
guard is often a _nothrow_ operation. Notice in particular that
`make_scope_guard` is `noexcept` in the following cases:

1. when `callback` is an lvalue or lvalue reference (`Callback` deduced to be
a reference)
2. when `callback` is an rvalue or rvalue reference of a type with a `noexcept`
move constructor (and a `noexcept` destructor, but that is already REQUIRED by
the [preconditions](#preconditions-in-detail) in this case)

You can look for `noexcept` in [compilation tests](compile_time_tests.cpp) for
examples.

### No extra arguments

As the signature shows, `make_scope_guard` accepts no arguments beyond
the callback (see the [related precondition](#invocable-with-no-arguments)).
I could not see a compelling need for them. When lambdas and binds are
available, the scope guard is better off keeping a _single responsibility_ of
guarding the scope and leaving the complementary responsibility of closure to
those other types.

### Private constructor

The form of construction that is used by `make_scope_guard` to create scope
guards is not part of the public interface. The purpose is to guide the user to
type deduction with universal references and make unintentional misuse difficult
(e.g. dynamic allocation of scope_guards).

### SFINAE friendliness

The function `make_scope_guard` is _SFINAE-friendly_. In other words, when the
compiler tries to deduce a template argument, an invalid application of
`make_scope_guard` that is caused by a failure to substitute a candidate type
(e.g. because the argument is not callable) does not cause a compilation error
if any other substitution is still possible.

You can look for "sfinae" in [catch tests](catch_tests.cpp) for examples.
