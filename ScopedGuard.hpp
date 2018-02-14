/*
 *  Created on: 13/02/2018
 *      Author: ricab
 */

#ifndef SCOPEDGUARD_HPP_
#define SCOPEDGUARD_HPP_

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
  class ScopeGuard
  {
  public:
    template<typename = typename std::enable_if<
      std::is_constructible<std::function<void()>, Callable>::value>::type>
    explicit ScopeGuard(Callable&& resetter);
    ScopeGuard(ScopeGuard&& other);
    ~ScopeGuard();

  private:
    bool m_active;
    Callable m_resetter;
  };

  /// helper to create ScopeGuard and deduce template params
  template<typename Callable>
  ScopeGuard<Callable> make_scope_guard(Callable&& resetter);
}

////////////////////////////////////////////////////////////////////////////////
template<typename Callable>
template<typename>
gproj::ScopeGuard<Callable>::ScopeGuard(Callable&& resetter)
  : m_active{true}
  , m_resetter{std::forward<Callable>(resetter)}
{}

////////////////////////////////////////////////////////////////////////////////
template<typename Callable>
gproj::ScopeGuard<Callable>::~ScopeGuard()
{
  if(m_active)
    m_resetter();
}

////////////////////////////////////////////////////////////////////////////////
template<typename Callable>
gproj::ScopeGuard<Callable>::ScopeGuard(ScopeGuard&& other)
  : m_active{other.m_active}
  , m_resetter{std::move(other.m_resetter)}
{
  other.m_active = false;
}

////////////////////////////////////////////////////////////////////////////////
template<typename Callable>
inline auto gproj::make_scope_guard(Callable&& resetter) -> ScopeGuard<Callable>
{
  return ScopeGuard<Callable>{std::forward<Callable>(resetter)};
}

#endif /* SCOPEDGUARD_HPP_ */
