/*
 *  Created on: 27/02/2018
 *      Author: ricab
 */

#if __cplusplus >= 201703L && defined(SG_REQUIRE_NOEXCEPT_IN_CPP17)
#define SG_REQUIRE_NOEXCEPT 1
#else
#define SG_REQUIRE_NOEXCEPT 0
#endif

#define SG_ENABLE(test) !SG_REQUIRE_NOEXCEPT || test

#include "scope_guard.hpp"

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

//  /**
//   * Things that should fail compilation when noexcept is required and pass
//   * compilation otherwise, resulting in a call to std::terminate
//   */
//  namespace test_bad
//  {
//    void test()
//    {
//      make_scope_guard(throwing);
//      make_scope_guard(throwing_stdfun);
//      make_scope_guard(throwing_lambda);
//      make_scope_guard(throwing_bound);
//      make_scope_guard(throwing_functor);
//    }
//  }
//
//  /**
//   * Things that should fail compilation when noexcept is required, even
//   * though they don't throw, because they should have been marked noexcept but
//   * weren't
//   */
//  namespace test_fixable
//  {
//    void test()
//    {
//      make_scope_guard(meh);
//      make_scope_guard(meh_stdfun);
//      make_scope_guard(meh_lambda);
//      make_scope_guard(meh_bound);
//      make_scope_guard(meh_functor);
//    }
//  }
//
//  /**
//   * Things that should be ok but aren't because they cannot be marked noexcept
//   */
//  namespace test_unfortunate
//  {
//    void test()
//    {
//      make_scope_guard(non_throwing_stdfun);
//      make_scope_guard(non_throwing_bound);
//    }
//  }
//
//
//  /**
//   * Things that should pass compilation and run successfully in all cases
//   */
//  namespace test_good
//  {
//    void test()
//    {
//      make_scope_guard(non_throwing);
//      make_scope_guard(non_throwing_lambda);
//      make_scope_guard(non_throwing_functor);
//    }
//  }
}

int main()
{
//  test_good::test();
//
//#if __cplusplus < 201703L || !SG_REQUIRE_NOEXCEPT_IN_CPP17
//  test_fixable::test();
//  test_unfortunate::test();
//#endif
//
//  // test_bad::test(); // this would result in a call to std::terminate

  return 0;
}
