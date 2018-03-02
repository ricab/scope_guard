/*
 *  Created on: 13/02/2018
 *      Author: ricab
 */

#ifndef SCOPE_GUARD_HPP_
#define SCOPE_GUARD_HPP_

#include <type_traits>
#include <functional>
#include <utility>

#if __cplusplus >= 201703L && defined(SG_REQUIRE_NOEXCEPT_IN_CPP17)
#define SG_REQUIRE_NOEXCEPT
#endif

////////////////////////////////////////////////////////////////////////////////
namespace sg
{
  namespace detail
  {
    // Type trait determining whether a type is a proper scope_guard callback.
    template<typename T>
    struct is_proper_sg_callback : public
#ifdef SG_REQUIRE_NOEXCEPT
      std::is_nothrow_invocable_r<void, T>
#else
      std::is_constructible<std::function<void()>, T>
#endif
    {};

    // The actual scope guard type
    template<typename Callback>
    class scope_guard
    {
    public:
      template<typename = typename std::enable_if<
        is_proper_sg_callback<Callback>::value>::type>
      explicit scope_guard(Callback&& callback) noexcept;

      scope_guard(scope_guard&& other) noexcept;
      ~scope_guard() noexcept; // highlight noexcept dtor

    private:
      Callback m_callback;
      bool m_active;

    };

  } // namespace detail

  /* -- now the single public maker function -- */
  /**
   * Function to create a scope_guard.
   *
   * @param callback A callable function, function pointer, functor, or
   * reference thereof, that must:
   * @li require no parameters;
   * @li return void;
   * @li not throw.
   * The latter is not enforced upon compilation unless >=C++17 is used and the
   * preprocessor macro SG_REQUIRE_NOEXCEPT_IN_CPP17 is defined. If the callback
   * throws, std::terminate is called. @note check the documentation in the
   * readme for more details).
   *
   * @return A scope_guard - an RAII object that executes a provided callback
   * when leaving scope.
   */
  template<typename Callback>
  detail::scope_guard<Callback> make_scope_guard(Callback&& callback) noexcept;

} // namespace sg

////////////////////////////////////////////////////////////////////////////////
template<typename Callback>
template<typename>
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
