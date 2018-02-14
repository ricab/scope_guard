/*
 *  Created on: 13/02/2018
 *      Author: ricab
 */

#ifndef SCOPED_GUARD_HPP_
#define SCOPED_GUARD_HPP_

#include <type_traits>
#include <functional>
#include <utility>

////////////////////////////////////////////////////////////////////////////////
namespace gproj
{
  /**
   * RAII class to call a resetter function when leaving scope. The resetter
   * needs to be compatible with std::function<void()>.
   */
  template<typename Callable>
  class scope_guard
  {
  public:
    template<typename = typename std::enable_if<
      std::is_constructible<std::function<void()>, Callable>::value>::type>
    explicit scope_guard(Callable&& resetter);
    scope_guard(scope_guard&& other);
    ~scope_guard();

  private:
    bool m_active;
    Callable m_resetter;
  };

  /// helper to create scope_guard and deduce template params
  template<typename Callable>
  scope_guard<Callable> make_scope_guard(Callable&& resetter);
}

////////////////////////////////////////////////////////////////////////////////
template<typename Callable>
template<typename>
gproj::scope_guard<Callable>::scope_guard(Callable&& resetter)
  : m_active{true}
  , m_resetter{std::forward<Callable>(resetter)}
{}

////////////////////////////////////////////////////////////////////////////////
template<typename Callable>
gproj::scope_guard<Callable>::~scope_guard()
{
  if(m_active)
    m_resetter();
}

////////////////////////////////////////////////////////////////////////////////
template<typename Callable>
gproj::scope_guard<Callable>::scope_guard(scope_guard&& other)
  : m_active{other.m_active}
  , m_resetter{std::move(other.m_resetter)}
{
  other.m_active = false;
}

////////////////////////////////////////////////////////////////////////////////
template<typename Callable>
inline auto gproj::make_scope_guard(Callable&& resetter) -> scope_guard<Callable>
{
  return scope_guard<Callable>{std::forward<Callable>(resetter)};
}

#endif /* SCOPED_GUARD_HPP_ */
