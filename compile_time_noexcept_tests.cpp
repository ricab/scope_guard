/*
 *  Created on: 27/02/2018
 *      Author: ricab
 */

#include "scope_guard.hpp"
using namespace sg;

////////////////////////////////////////////////////////////////////////////////
namespace
{
  constexpr auto EMSG = "message in a bottle";
  [[noreturn]] void throwing() { throw std::runtime_error{EMSG}; }
  void non_throwing() noexcept { }
  void meh() { }

  using StdFun = std::function<void()>;
  StdFun throwing_stdfun{throwing};
  StdFun non_throwing_stdfun{non_throwing}; // drops noexcept
  StdFun meh_stdfun{meh};

  auto throwing_lambda = [](){ throwing(); };
  auto non_throwing_lambda = []() noexcept { non_throwing(); };
  auto meh_lambda = [](){ meh(); };

  auto throwing_bound = std::bind(throwing);
  auto non_throwing_bound = std::bind(non_throwing); // drops noexcept
  auto meh_bound = std::bind(meh);

  struct throwing_struct
  {
    void operator()() { throwing(); }
  } throwing_functor;

  struct non_throwing_struct
  {
    void operator()() noexcept{ non_throwing(); }
  } non_throwing_functor;

  struct meh_struct
  {
    void operator()() { meh(); }
  } meh_functor;

  /**
   * Things that should pass compilation and run successfully in all cases
   */
  namespace test_good
  {
    void test()
    {
#if test_1
      make_scope_guard(non_throwing);
#endif
#if test_2
      make_scope_guard(non_throwing_lambda);
#endif
#if test_3
      make_scope_guard(non_throwing_functor);
#endif
    }
  }

  /**
   * Things that should fail compilation when noexcept is required and pass
   * compilation otherwise, resulting in a call to std::terminate
   */
  namespace test_bad
  {
    void test()
    {
#if test_4
      make_scope_guard(throwing);
#endif
#if test_5
      make_scope_guard(throwing_stdfun);
#endif
#if test_6
      make_scope_guard(throwing_lambda);
#endif
#if test_7
      make_scope_guard(throwing_bound);
#endif
#if test_8
      make_scope_guard(throwing_functor);
#endif
    }
  }

  /**
   * Things that should fail compilation when noexcept is required, even
   * though they don't throw, because they should have been marked noexcept but
   * weren't
   */
  namespace test_fixable
  {
    void test()
    {
#if test_9
      make_scope_guard(meh);
#endif
#if test_10
      make_scope_guard(meh_stdfun);
#endif
#if test_11
      make_scope_guard(meh_lambda);
#endif
#if test_12
      make_scope_guard(meh_bound);
#endif
#if test_13
      make_scope_guard(meh_functor);
#endif
    }
  }

  /**
   * Things that should be ok but aren't because they cannot be marked noexcept
   */
  namespace test_unfortunate
  {
    void test()
    {
#if test_14
      make_scope_guard(non_throwing_stdfun);
#endif
#if test_15
      make_scope_guard(non_throwing_bound);
#endif
    }
  }
}

int main()
{
  test_good::test();
  test_fixable::test();
  test_unfortunate::test();

  // test_bad::test(); // this would result in a call to std::terminate

  return 0;
}
