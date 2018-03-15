/*
 *  Created on: 13/02/2018
 *      Author: ricab
 */

#ifndef SCOPE_GUARD_HPP_
#define SCOPE_GUARD_HPP_

#include <type_traits>
#include <utility>

#if __cplusplus >= 201703L && defined(SG_REQUIRE_NOEXCEPT_IN_CPP17)
#define SG_REQUIRE_NOEXCEPT
#endif

/**
 * Namespace for the scope_guard
 */
namespace sg
{
  namespace detail
  {
    /* --- Some custom type traits --- */

    // Type trait determining whether a type is callable with no arguments
    template<typename T, typename = void>
    struct is_noarg_callable_t
      : public std::false_type
    {};

    template<typename T>
    struct is_noarg_callable_t<T, decltype(std::declval<T&&>()())>
      : public std::true_type
    {};

    /* Type trait determining whether a no-arg callable is nothrow. This is
       where SG_REQUIRE_NOEXCEPT logic is encapsulated. */
    template<typename T>
    struct is_nothrow_if_required_t
      : public
#ifdef SG_REQUIRE_NOEXCEPT
      std::is_nothrow_invocable<T> /* Have C++17, so can use this directly.
                                      Note: _r variants not enough for our
                                      purposes: any return can be discarded
                                      so all returns are compatible with void */
#else
      std::true_type
#endif
    {};

    // Type trait determining whether a no-argument callable returns void
    template<typename T>
    struct returns_void_t
      : public std::is_same<void, decltype(std::declval<T&&>()())>
    {};

    // logic AND of two or more type traits
    template<typename A, typename B, typename... C>
    struct and_t : public and_t<A, and_t<B, C...>>
    {}; // for more than two arguments

    template<typename A, typename B>
    struct and_t<A, B> : public std::conditional<A::value, B, A>::type
    {}; // for two arguments

    // Type trait determining whether a type is a proper scope_guard callback.
    template<typename T>
    struct is_proper_sg_callback_t
      : public and_t<is_noarg_callable_t<T>,
                     is_nothrow_if_required_t<T>,
                     returns_void_t<T>>
    {};


    /* --- The actual scope_guard type --- */

    template<typename Callback,
             typename = typename std::enable_if<
               is_proper_sg_callback_t<Callback>::value>::type>
    class scope_guard;

    template<typename Callback>
    class scope_guard<Callback>
    {
    public:
      typedef Callback callback_type;

      explicit scope_guard(Callback&& callback) noexcept;

      scope_guard(scope_guard&& other) noexcept;
      ~scope_guard() noexcept; // highlight noexcept dtor

      scope_guard(const scope_guard&) = delete;
      scope_guard& operator=(const scope_guard&) = delete;
      scope_guard& operator=(scope_guard&&) = delete;

    private:
      Callback m_callback;
      bool m_active;

    };

  } // namespace detail


  /* --- Now the single public maker function --- */

  /**
   * Function template to create a scope_guard.
   *
   * @param callback A callable function, function pointer, functor, or
   * reference thereof, that must:
   * @li require no parameters;
   * @li return void;
   * @li not throw.
   * @attention The latter is not enforced upon compilation unless >=C++17 is
   * used and the preprocessor macro SG_REQUIRE_NOEXCEPT_IN_CPP17 is defined.
   * If the callback throws, std::terminate is called.
   * @note Check the documentation in @c README.md for more details).
   *
   * @return A scope guard, that is, an RAII object that executes the provided
   * callback when leaving scope.
   */
  template<typename Callback>
  detail::scope_guard<Callback> make_scope_guard(Callback&& callback) noexcept;

} // namespace sg

////////////////////////////////////////////////////////////////////////////////
template<typename Callback>
sg::detail::scope_guard<Callback>::scope_guard(Callback&& callback) noexcept
  : m_callback{std::forward<Callback>(callback)}
  , m_active{true}
{}

////////////////////////////////////////////////////////////////////////////////
template<typename Callback>
sg::detail::scope_guard<Callback>::~scope_guard() noexcept
{
  if(m_active)
    m_callback();
}

////////////////////////////////////////////////////////////////////////////////
template<typename Callback>
sg::detail::scope_guard<Callback>::scope_guard(scope_guard&& other) noexcept
  : m_callback{std::forward<Callback>(other.m_callback)}
  , m_active{std::move(other.m_active)}
{
  other.m_active = false;
}

////////////////////////////////////////////////////////////////////////////////
template<typename Callback>
inline auto sg::make_scope_guard(Callback&& callback) noexcept
-> detail::scope_guard<Callback>
{
  return detail::scope_guard<Callback>{std::forward<Callback>(callback)};
}

#endif /* SCOPE_GUARD_HPP_ */
