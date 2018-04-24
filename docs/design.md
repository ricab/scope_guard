## Design choices and concepts

This section tries to clarify concepts used in the interface and discusses the
rationale for some design decisions. The outline is:

- [No exceptions](#no-exceptions)
- [Implications of requiring `noexcept` callbacks at compile time](#implications-of-requiring-noexcept-callbacks-at-compile-time)
- [No return](#no-return)
- [Conditional `noexcept`](#conditional-noexcept)
- [No extra arguments](#no-extra-arguments)
- [Private constructor](#private-constructor)
- [Unspecified type](#unspecified-type)
- [No default constructor or assignment operator](#no-default-constructor-or-move-assignment-operator)
- [SFINAE friendliness](#sfinae-friendliness)

### No exceptions

Scope guards rely on the destructor to execute the client's operation. That
elicits special consideration regarding possible callback exceptions. Possible
options to deal with potential exceptions arising from a callback (or a
callback's destruction) are:

1. Allowing exceptions and...
    1. ... always ignoring (silently, with logging...)
    2. ... always aborting (with std::terminate)
    3. ... relegating handling to the caller (rethrowing).
    4. ... providing a way for the client to convey a handling policy to the
    scope guard (1, 2, 3, or some custom operation)
2. Forbidding exceptions in the first place.

###### Options 1.1 and 1.2 - uniform handling

Exceptions permit custom handling of distinct errors by higher level callers,
so they can make use of higher level information that is only available to them.
Uniform default handling methods would contradict that purpose and should in
principle be avoided. That advises against options 1.1 and 1.2. Specifically,
ignoring errors in a systematic manner is hardly a reasonable option. Idem for
allowing exceptions only to abort when they arise, which would transmit
conflicting ideas to the user.

###### Option 1.3 (and 1.2 fallback) - rethrowing

A crucial property of exceptions is that they propagate automatically, freeing
intermediaries from boilerplate code. Option 1.3 exploits that property and
is usually the ideal default. To the point that no special code needs to be
written for it (rethrowing happens automatically). Destructors are special
though. Alas, _exceptions do not always propagate_. In particular, they do not
propagate when the stack is already unwinding. To put it another way, exception
propagation is really only achievable from code that is not already being
executed in the context of exception propagation. Which is one of the
circumstances where scope guards are most useful. In those cases, the program
aborts, meaning that option 1.3 implies falling back on 1.2.

Still, option 1.3 (with 1.2 fallback) is appealling. It can be implemented
simply with a destructor that a) is not `noexcept` when the callback isn't
either; and b) leaves exceptions to behave as they do by default. In other
words, by just declaring the destructor `noexcept(noexcept(callback()))` and
otherwise leaving exceptions alone. This would allow using scope guards for
exception throwing in regular cases (non-stack-unwinding). It is the path
followed by other implementations, including
[N4189](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4189.pdf)
(section 7.3). One I considered seriously.

However, I ended up deciding against it. It would create a false sense of
safety and lead to hidden _crashes_. It is easy to imagine situations with
callbacks happily throwing under regular test conditions, only to cause crashes
in production. Better _fail-fast_ to discourage the client from relying on, or
otherwise using, throwing callbacks. After all, throwing destructors are
considered bad practice for a reason.

###### Option 1.4 - policy

Option 1.4 could be achieved with some additional policy parameter for the
client's choice, but that would complicate an interface that is desirably
simple. If there was no default, the client would have to choose a policy even
if no exceptions were involved. With a default, the client would need to
remember considering it in each case. And, critically, choosing a default means
recurring into the present problem &ndash; of how to deal with exceptions in the
first place &ndash; which does not have a satisfactory answer so far.

###### Option 2 - forbidding exceptions

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
option is disabled by default. I personally favor enabling it, if possible (in
&ge;C++17).

### No return

A scope guard cannot know what to do with any callback returns. It can only
ignore and discard them. But returns express information that is usually
important. They are often employed for error handling and even to transfer
resource ownership. Forgetting to check returns is a familiar cause of bugs.
Linters look for them and compilers provide special attributes to warn against
them (e.g
[warn_unused_result](https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html)).
The attribute [[nodiscard]] was even added to C++17 to help addressing such
issues.

The situation is still suboptimal, in my opinion. The decision of whether to
ignore a return should belong to the caller, not the provider. But it should be
an explicit decision (just like casts for conversions). In an ideal world,
ignored returns should never have been possible without a conscious request
from the programmer that communicated intent to both the compiler and the
reader.

Unfortunately that is not how things evolved and existing code relies on
existing rules. It is not so for new code, which is free to make improvements.
That is the intention behind the "void return" requirement here. It forces the
client to confirm their intention, by explicitly writing code to
[ignore a return](precond.md#void-return), if that really is what they want. It
catches unintentional cases and highlights intentional ones for the reader.

Downsides have to be weighter too, but in this case I see only two and I don't
think they are determinant:

1. consistency
2. performance

Regarding 1, one could argue that the normal programmer expectation is that
returns can be ignored, taking into account not only language rules but also
existing APIs. For instance, I realize that the option in `std::thread` and
`std::function` is to allow returns:

```c++
bool f() { return false; }
...
std::thread t{f}; // Accepted, return is lost
auto r2 = t.join(); // ERROR, no good, returns void
std::function<void()> wrap{f}; // Accepted, return is lost
auto r1 = wrap(); // ERROR, wrap does not return anything
```

However, while consistency is a desirable property, it is hardly a critical one
for new undertakings. And it is frequently incompatible with improvement &ndash;
preventing change also stalls advance. Asking the reader not to take offense at
the title and headings, I suggest a look at
[Jon Kalb's post](http://slashslash.info/2018/02/a-foolish-consistency/)
on the matter, which guides my own perspective and is much more eloquent that I
could hope to be.

Notice that since it is enforced at compile-time, the user is quickly alerted
to the approach taken here, so he cannot commit to unintentional mistakes or be
surprised by runtime problems, which compile time errors mean to avoid.

Concerning 2, it is true that the scope guards will require some returning
operations to be wrapped in an additional indirection. Function casts can't help
and the only way to use a returning function is to wrap it in a non-returning
callable. There are no guarantees that the compiler will inline or optimize the
wrapping away, so there may be a non-null performance penalty, even if a tiny
one.

That is the price to pay. I view it as analogous to the price that is payed when
using std::string rather than char[], vector instead of plain arrays, or lambdas
instead of functions. As in those cases, it is negligible and almost always
worth it. In the rare occasions where measuring shows that price to be
significant, a scope guard is not the right tool. The concept of scope guard
always proposes wrapping as a means of avoiding repetition and corner-case
pitfalls. If you can't pay the price of a function wrapping, you should
probably take the care to ensure manual function calls in every scope exit path
rather than using a scope guard to begin with.

### Conditional `noexcept`

Conditional `noexcept`'s are essential for correct exception specifications in
modern C++ and could not be neglected by a construct that is particularly
useful to achieve exception safety. Taking into account that they are a
relatively recent feature that many programmers are not used to, `noexcept`'s
make the code more verbose and arguably harder to read, especially when the
condition uses type traits. But they could not be discarded by an interface that
has strong non-throwing requirements on input. Hopefully, the way the code
separates function declarations and definitions compensates on readability.

The exception specification of `make_scope_guard` is such that making a scope
guard is often a _nothrow_ operation. Notice in particular that
`make_scope_guard` is `noexcept` in the following cases:

1. when `callback` is an lvalue or lvalue reference (`Callback` deduced to be
a reference)
2. when `callback` is an rvalue or rvalue reference of a type with a `noexcept`
move constructor (and a `noexcept` destructor, but that is already
[required](precond.md#nothrow-destructible-if-non-reference-template-argument)
in such cases)

You can look for `noexcept` in [compilation tests](../compile_time_tests.cpp)
for examples of `make_scope_guard` instantiations with and without `noexcept`
guarantee.

### No extra arguments

As the signature shows, `make_scope_guard` accepts no arguments beyond
the callback (see the
[related precondition](precond.md#invocable-with-no-arguments)).
This makes the interface (and implementation) simpler and pure and I could not
see any other compelling case for them.

When lambdas and binds are available, the scope guard is better off keeping the _single responsibility_ of guarding the scope, leaving the complementary responsibility of closure, with all intricacies it involves (think lambda
captures), to those other types.

So, operations that require arguments can be used indirectly, with some form of
closure. As with the [decision to forbid returns](#no-return), the potential
performance impacts of an additional wrapping level are hardly significant and I
don't think they are a decisive factor.

### Private constructor

The form of construction that is used by `make_scope_guard` to create scope
guards is not part of the public interface. The purpose is to guide the user to
type deduction with universal references and make unintentional misuse
difficult. For instance, scope guards are not meant for dynamic allocation, or
they would not be guarding any scope. Making the callback-taking constructor
private does not make dynamic allocation impossible (because of the move
constructor, which remains public), but it makes it harder.

The same is true with other misuses, namely when bypassing type deduction. If
the callback-taking constructor was public, the user might be tempted to use it
in place of the maker function. He would then have to manually specify a
template instantiation that would be easy to get wrong. For example:

```c++
scope_guard<A&&>{std::move(a)}; /* oops, nothing is moved, may end up with
dangling ref (should be scope_guard<A>, which the compiler deduces when
using maker) */
```

### Unspecified type

I confess the decision not to specify the type of scope guards and keep it as an
implementation detail is one I am less sure of. The intention is the same as
with the private constructor: to make misuse hard. It discourages composition
into other types and other cases where the notion of scope is unclear. And it is
one less detail for the client to depend on. It is an approach I have seen on
occasion, but the main inspiration comes from lambdas. I am open to strong
arguments to the contrary.

### No default constructor or move assignment operator

As they stand, inactive scope guards exist for only two purposes:

- to allow [dismissal](interface.md#member-function-dismiss)
- to allow [moving](interface.md#member-move-constructor)

Both intents are valid, and they justify supporting a special state that
is otherwise artificial and purely a technical matter. But this state exists
only because of the way C++ works. If those operations ended up with no scope
guard at all, their purpose would be equally fulfilled.

In other words, the inactive state represents nothing useful from any problem
domain. It would not be required in a hypothetical programming language where
local variables could be deleted early and moved without leaving _ghosts_
behind. So, we use the inactive state here only to enable valid programming
approaches that would otherwise not be possible in C++. But then, should we take
it one step further and use this state to provide another common C++ operation
&ndash; default construction?

Without any meaningful default callback, a potential default constructor could
only create inactive scope guards (or active no-op, which would be equivalent).
If they represent nothing useful from the problem domain, the only reason to
create them directly in such a state would be to provide placeholders to allow
modification. That is, to artificially _defer initialization_, which would then
have to be achieved with assignment, since no other meaningful scope guard
modifier is supported.

But having constructed objects that are not initialized is a practice that goes
against a very important principle! The one that corresponds to the literal
meaning of a well-known overloaded expression &ndash; _Resourse Acquisition Is
Initialization_. Constructors are meant to prevent uninitialized variables, not
facilitate them.

Additionally, unlike in the case of the move constructor, I see no technical
reason to provide the default constructor plus assignment operator combo. True,
it could enable the following:

```c++
{
  Foo callback;
  decltype(make_scope_guard(std::move(callback))) g;
  if(condition)
    g = make_scope_guard(std::move(callback));
  // g still alive
} // callback executed if condition held
```

But there are better alternatives to achieve the same thing without so many
pitfalls. Namely, using `std::optional`:

```c++
{
  Foo callback;
  std::optional<decltype(make_scope_guard(std::move(callback)))> g;
  if(condition)
    g.emplace(make_scope_guard(std::move(callback)));
  // g still alive
} // callback executed if condition held
```

Or moving the condition into the callback (if it does not change until then):

```c++
{
  Foo callback;
  auto g =
    make_scope_guard([&callback](){ if(condition) callback(); });
} // callback executed if condition holds (different moment)
```

### SFINAE friendliness

The function `make_scope_guard` is _SFINAE-friendly_. In other words, when the
compiler tries to deduce a template argument, an invalid application of
`make_scope_guard` that is caused by a failure to substitute a candidate type
(e.g. because the argument is not callable) does not cause a compilation error
if any other substitution is still possible. You can look for "sfinae" in
[catch tests](../catch_tests.cpp) for examples.

Making the scope guard SFINAE-friendly is the decision I am less sure of. It
_felt right_, but it makes error output unclear. I welcome justified opinions
and improvement suggestions (on this topic as in others).