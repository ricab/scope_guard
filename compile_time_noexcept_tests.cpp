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

  auto throwing_lambda = []{ throwing(); };
  auto non_throwing_lambda = []() noexcept { non_throwing(); };
  auto meh_lambda = []{ meh(); };

  auto throwing_bound = std::bind(throwing);
  auto non_throwing_bound = std::bind(non_throwing); // drops noexcept
  auto meh_bound = std::bind(meh);

  struct throwing_struct
  {
    [[noreturn]] void operator()() { throwing(); }
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
    inline void test() // inlining to avoid missing prototype warning
    {
#ifdef test_1
      make_scope_guard(non_throwing);
#endif
#ifdef test_2
      make_scope_guard(non_throwing_lambda);
#endif
#ifdef test_3
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
    inline void test()
    {
#ifdef test_4
      make_scope_guard(throwing);
#endif
#ifdef test_5
      make_scope_guard(throwing_stdfun);
#endif
#ifdef test_6
      make_scope_guard(throwing_lambda);
#endif
#ifdef test_7
      make_scope_guard(throwing_bound);
#endif
#ifdef test_8
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
    inline void test()
    {
#ifdef test_9
      make_scope_guard(meh);
#endif
#ifdef test_10
      make_scope_guard(meh_stdfun);
#endif
#ifdef test_11
      make_scope_guard(meh_lambda);
#endif
#ifdef test_12
      make_scope_guard(meh_bound);
#endif
#ifdef test_13
      make_scope_guard(meh_functor);
#endif
    }
  }

  /**
   * Things that should be ok but aren't because they cannot be marked noexcept
   */
  namespace test_unfortunate
  {
    inline void test()
    {
#ifdef test_14
      make_scope_guard(non_throwing_stdfun);
#endif
#ifdef test_15
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
