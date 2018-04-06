## Design choices and concepts

This section tries to clarify concepts used in the interface and discusses the
rationale for some design decisions. The outline is:

- [No exceptions](#no-exceptions)
- [Implications of requiring `noexcept` callbacks at compile time](#implications-of-requiring-noexcept-callbacks-at-compile-time)
- [No return](#no-return)
- [Conditional `noexcept`](#conditional-noexcept)
- [No extra arguments](#no-extra-arguments)
- [Private constructor](#private-constructor)
- [SFINAE friendliness](#sfinae-friendliness)

### No exceptions

Scope guards rely on the destructor to execute the client's operation. That
elicits special consideration regarding exceptions. Possible options to deal
with potential exceptions arising from a callback (or a callback's destruction)
are:

1. Allowing exceptions and...
    1. ... always ignoring (silently, with logging...)
    2. ... always aborting (with std::terminate)
    3. ... relegating handling to the caller (rethrowing).
    4. ... providing a way for the client to convey a handling policy to the
    scope guard (1, 2, 3, or some custom operation)
2. Forbidding exceptions in the first place.

###### Options 1.1 and 1.2

Exceptions permit custom handling of distinct errors by higher level callers,
using information available to them. Uniform default handling methods would
contradict that purpose and should in principle be avoided. That advises against
options 1.1 and 1.2. Ignoring errors systematically is hardly a reasonable
option. Allowing exceptions only to abort when they arise would transmit
conflicting ideas to the user.

###### Options 1.3 and 1.2

A crucial property of exceptions is that they propagate automatically, freeing
intermediaries from boilerplate code. Option 1.3 makes use of that property and
may therefore seem like the ideal default. Alas, _exceptions do not always
propagate_. In particular, they do not propagate when the stack is already
unwinding. To put it another way, exception propagation is really only
achievable from code that is not already being executed in the context of
exception propagation. Which is one of the circumstances where scope guards are
most useful. In those cases, the program aborts, meaning that option 1.3 implies
falling back to 1.2.

Still, option 1.3 (with 1.2 fallback) is appealling. It can be implemented
simply with a destructor that a) is not `noexcept` when the callback isn't
either; and b) leaves exceptions to behave as they do by default. It allows
using scope guards for exception throwing in regular cases and it is the most
common approach in implementations I have seen, including
[N4189](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4189.pdf)
(section 7.3). One I considered seriously.

However, I ended up deciding against it. It would create a false sense of
safety and lead to hidden _crashes_. It is easy to imagine situations with
callbacks happily throwing under regular test conditions, only to cause crashes
in production. Better _fail-fast_ to discourage the client from relying on, or
otherwise using, throwing callbacks. After all, throwing destructors are
considered bad practice for a reason.

###### Option 1.4

Option 1.4 could be achieved with some additional policy parameter for the
client's choice, but that would complicate an interface that is desirably
simple. If there was no default, the client would have to choose a policy even
if no exceptions were involved. With a default, the client would need to
remember considering it in each case. And, critically, choosing a default means
recurring into the present problem &ndash; of how to deal with exceptions in the
first place &ndash; which does not have a satisfactory answer so far.

###### Option 2

That leaves option 2, which is the one I went for. It still allows throwing
operations to be used indirectly, if they are _converted_ to non-throwing. That
can be achieved [easily](precond.md#nothrow-invocable) with a try-catch wrapping
that ignores (as in 1.1), aborts (as in 1.2) or chooses a custom way of handling
the exception (as in 1.4), as long as it does not require throwing. Nothing like
1.3 is possible within option 2 as, by definition, it precludes uses involving
throwing. But that is perhaps a sensible target, taking into account the
arguments above.

An [option](interface.md/#compilation-option-sg_require_noexcept_in_cpp17) to
enforce this at compilation is available, but requires C++17 and has
[other implications](#implications-of-requiring-noexcept-callbacks-at-compile-time).
In the absence of compile-time checking, if a callback happens to throw
_by mistake_, the program aborts via a call to `std::terminate` (the dtor is
declared `noexcept`).

The distinction between option 2 and 1.2 is important, even if subtle: here the
specification expressly places a burden of ensuring non-throwing callbacks on
the client, making it clear they cannot be expected to work. This follows the
same approach as custom deleters in such standard library types as `unique_ptr`
and `shared_ptr` (see `[unique.ptr.single.ctor]` and
`[unique.ptr.single.dtor]` in the C++ standard).

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

Therefore, such types cannot be used directly to create scope guards when
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
