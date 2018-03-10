/*
 *  Created on: 27/02/2018
 *      Author: ricab
 */

#include "scope_guard.hpp"
#include <utility>
using namespace sg;

////////////////////////////////////////////////////////////////////////////////
#ifdef test_1
  static_assert(noexcept(make_scope_guard(std::declval<void(*)()noexcept>())),
                "make_scope_guard not noexcept");
#endif

#ifdef test_2
  static_assert(noexcept(detail::scope_guard<void(*)()noexcept>{
    std::declval<void(*)()noexcept>()}), "scope_guard ctor not noexcept");
#endif

#ifdef test_3
  static_assert(noexcept(make_scope_guard(std::declval<void(*)()noexcept>())
                         .~scope_guard()),
                "scope_guard dtor not noexcept");
#endif

////////////////////////////////////////////////////////////////////////////////
namespace
{

  constexpr auto EMSG = "message in a bottle";
  void non_throwing() noexcept { }
  [[noreturn]] void throwing() { throw std::runtime_error{EMSG}; }
  void meh() { }

  int returning() noexcept { return 42; }


  using StdFun = std::function<void()>;
  StdFun throwing_stdfun{throwing};
  StdFun non_throwing_stdfun{non_throwing}; // drops noexcept
  StdFun meh_stdfun{meh};

  std::function<int()> returning_stdfun{returning}; // drops noexcept


  auto throwing_lambda = []{ throwing(); };
  auto non_throwing_lambda = []() noexcept { non_throwing(); };
  auto meh_lambda = []{ meh(); };

  auto returning_lambda = []() noexcept { return returning(); };


  auto throwing_bound = std::bind(throwing);
  auto non_throwing_bound = std::bind(non_throwing); // drops noexcept
  auto meh_bound = std::bind(meh);

  auto returning_bound = std::bind(returning); // drops noexcept


  struct throwing_struct
  {
    [[noreturn]] void operator()() { throwing(); }
  } throwing_functor;
  struct non_throwing_struct
  {
    void operator()() noexcept { non_throwing(); }
  } non_throwing_functor;
  struct meh_struct
  {
    void operator()() { meh(); }
  } meh_functor;

  struct returning_struct
  {
    int operator()() noexcept { return returning(); }
  } returning_functor;

  /**
   * Test scope_guard can always be created with noexcept marked callables
   */
  void test_noexcept_good()
  {
#ifdef test_4
    make_scope_guard(non_throwing);
#endif
#ifdef test_5
    make_scope_guard(non_throwing_lambda);
#endif
#ifdef test_6
    make_scope_guard(non_throwing_functor);
#endif
  }

  /**
   * Highlight that scope_guard should not be created with throwing callables,
   * under penalty of an immediate std::terminate call. Test that compilation
   * fails on such an attempt when noexcept is required.
   */
  void test_noexcept_bad()
  {
#ifdef test_7
    make_scope_guard(throwing);
#endif
#ifdef test_8
    make_scope_guard(throwing_stdfun);
#endif
#ifdef test_9
    make_scope_guard(throwing_lambda);
#endif
#ifdef test_10
    make_scope_guard(throwing_bound);
#endif
#ifdef test_11
    make_scope_guard(throwing_functor);
#endif
  }

  /**
   * Highlight the importance of declaring scope_guard callables noexcept
   * (when they do not throw). Test that compilation fails on attempts to the
   * contrary when noexcept is required.
   */
  void test_noexcept_fixable()
  {
#ifdef test_12
    make_scope_guard(meh);
#endif
#ifdef test_13
    make_scope_guard(meh_stdfun);
#endif
#ifdef test_14
    make_scope_guard(meh_lambda);
#endif
#ifdef test_15
    make_scope_guard(meh_bound);
#endif
#ifdef test_16
    make_scope_guard(meh_functor);
#endif
  }

  /**
   * Highlight that some callables cannot be declared noexcept even if they are
   * known not to throw. Show that trying to create scope_guards with such
   * objects unfortunately (but unavoidably AFAIK) breaks compilation when
   * noexcept is required
   */
  void test_noexcept_unfortunate()
  {
#ifdef test_17
    make_scope_guard(non_throwing_stdfun);
#endif
#ifdef test_18
    make_scope_guard(non_throwing_bound);
#endif
  }

  /**
   * Test that compilation fails when trying to copy-construct a scope_guard
   */
  void test_disallowed_copy_construction()
  {
    const auto guard = make_scope_guard(non_throwing);
#ifdef test_19
    const auto guard2 = guard1;
#endif
  }

  /**
   * Test that compilation fails when trying to copy-assign a scope_guard
   */
  void test_disallowed_copy_assignment()
  {
    const auto guard1 = make_scope_guard(non_throwing_lambda);
    auto guard2 = make_scope_guard(non_throwing_functor);
#ifdef test_20
    guard2 = guard1;
#endif
  }

  /**
   * Test that compilation fails when trying to move-assign a scope_guard
   */
  void test_disallowed_move_assignment()
  {
    auto guard = make_scope_guard(non_throwing);
#ifdef test_21
    guard = make_scope_guard(non_throwing_lambda);
#endif
  }

  /**
   * Test that compilation fails when trying to use a returning function to
   * create a scope_guard
   */
  void test_disallowed_return()
  {
#ifdef test_22
    make_scope_guard(returning);
#endif
#ifdef test_23
    make_scope_guard(returning_stdfun);
#endif
#ifdef test_24
    make_scope_guard(returning_lambda);
#endif
#ifdef test_25
    make_scope_guard(returning_bound);
#endif
#ifdef test_26
    make_scope_guard(returning_functor);
#endif
//    make_scope_guard([]() noexcept { return 42; }); // FIXME
  }
}

int main()
{
  test_noexcept_good();
  test_noexcept_fixable();
  test_noexcept_unfortunate();
  // test_noexcept_bad(); // this would result in a call to std::terminate

  test_disallowed_copy_construction();
  test_disallowed_copy_assignment();
  test_disallowed_move_assignment();
  test_disallowed_return();

  return 0;
}
