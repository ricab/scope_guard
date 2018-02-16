/*
 *  Created on: 13/02/2018
 *      Author: ricab
 */

#ifndef SCOPE_GUARD_HPP_
#define SCOPE_GUARD_HPP_

#include <type_traits>
#include <functional>
#include <utility>

////////////////////////////////////////////////////////////////////////////////
namespace sg
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
    Callable m_resetter;
    bool m_active;
  };

  /// helper to create scope_guard and deduce template params
  template<typename Callable>
  scope_guard<Callable> make_scope_guard(Callable&& resetter);
}

////////////////////////////////////////////////////////////////////////////////
template<typename Callable>
template<typename>
sg::scope_guard<Callable>::scope_guard(Callable&& resetter)
  : m_resetter{std::forward<Callable>(resetter)}
  , m_active{true}
{}

////////////////////////////////////////////////////////////////////////////////
template<typename Callable>
sg::scope_guard<Callable>::~scope_guard()
{
  if(m_active)
    m_resetter();
}

////////////////////////////////////////////////////////////////////////////////
template<typename Callable>
sg::scope_guard<Callable>::scope_guard(scope_guard&& other)
  : m_resetter{std::forward<Callable>(other.m_resetter)}
  , m_active{std::move(other.m_active)}
{
  other.m_active = false;
}

////////////////////////////////////////////////////////////////////////////////
template<typename Callable>
inline auto sg::make_scope_guard(Callable&& resetter) -> scope_guard<Callable>
{
  return scope_guard<Callable>{std::forward<Callable>(resetter)};
}

#endif /* SCOPE_GUARD_HPP_ */
