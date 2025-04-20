/*
 *  Created on: 27/02/2018
 *      Author: ricab
 */

#include "scope_guard.hpp"
#include <functional>
#include <stdexcept>
#include <tuple>
#include <utility>

using namespace sg;

/* --- first some test helpers --- */

////////////////////////////////////////////////////////////////////////////////
namespace
{
  constexpr auto EMSG = "message in a bottle";
  void non_throwing() noexcept { }
  [[noreturn]] void throwing() { throw std::runtime_error{EMSG}; }
  void meh() { }

  int returning() noexcept { return 42; }


  using StdFun = std::function<void()>;
  const StdFun throwing_stdfun{throwing};
  const StdFun non_throwing_stdfun{non_throwing}; // drops noexcept
  const StdFun meh_stdfun{meh};

  const std::function<int()> returning_stdfun{returning}; // drops noexcept


  const auto throwing_lambda = []{ throwing(); };
  const auto non_throwing_lambda = []() noexcept { non_throwing(); };
  const auto meh_lambda = []{ meh(); };

  const auto returning_lambda = []() noexcept { return returning(); };


  const auto throwing_bound = std::bind(throwing);
  const auto non_throwing_bound = std::bind(non_throwing); // drops noexcept
  const auto meh_bound = std::bind(meh);

  const auto returning_bound = std::bind(returning); // drops noexcept


  struct throwing_struct
  {
    [[noreturn]] void operator()() { throwing(); }
  } throwing_functor;
  struct non_throwing_struct
  {
    void operator()() const noexcept { non_throwing(); }
  } non_throwing_functor;
  struct meh_struct
  {
    void operator()() const { meh(); }
  } meh_functor;

  struct returning_struct
  {
    int operator()() const noexcept { return returning(); }
  } returning_functor;

  struct nocopy_nomove // non-copyable and non-movable
  {
    void operator()() const noexcept { non_throwing(); }

    nocopy_nomove() noexcept = default;
    ~nocopy_nomove() noexcept = default;

    nocopy_nomove(const nocopy_nomove&) = delete;
    nocopy_nomove& operator=(const nocopy_nomove&) = delete;
    nocopy_nomove(nocopy_nomove&&) = delete;
    nocopy_nomove& operator=(nocopy_nomove&&) = delete;
  };

  struct potentially_throwing_dtor
  {
    void operator()() const noexcept { non_throwing(); }

    ~potentially_throwing_dtor() noexcept(false) {}

    potentially_throwing_dtor() noexcept = default;
    potentially_throwing_dtor(const potentially_throwing_dtor&) noexcept
      = default;
    potentially_throwing_dtor(potentially_throwing_dtor&&) noexcept = default;
  };

  struct throwing_copy
  {
    void operator()() const noexcept { non_throwing(); }

    throwing_copy(const throwing_copy&) noexcept(false) {}

    ~throwing_copy() noexcept = default;
    throwing_copy() noexcept = default;
    throwing_copy(throwing_copy&&) noexcept = default;
  };

  struct throwing_move
  {
    void operator()() const noexcept { non_throwing(); }

    throwing_move(throwing_move&&) noexcept(false) {}

    ~throwing_move() noexcept = default;
    throwing_move() noexcept = default;
    throwing_move(const throwing_move&) noexcept = default;
  };

  struct nomove_throwing_copy
  {
    void operator()() const noexcept { non_throwing(); }

    nomove_throwing_copy(const nomove_throwing_copy&) noexcept(false) {}

    /*
     * nomove_throwing_copy(nomove_throwing_copy&&) noexcept;
     *
     * not declared! move ctor absent but not deleted - does not participate in
     * overload resolution, so copy ctor can still be selected
     */

    ~nomove_throwing_copy() noexcept = default;
    nomove_throwing_copy() noexcept = default;
  };

  struct nothrow
  {
    void operator()() const noexcept { non_throwing(); }

    ~nothrow() noexcept = default;
    nothrow() noexcept = default;
    nothrow(const nothrow&) noexcept = default;
    nothrow(nothrow&&) noexcept = default;
  };


  /* --- tests that always succeed --- */

#ifdef test_1
    static_assert(noexcept(make_scope_guard(std::declval<void(*)()noexcept>())),
                  "make_scope_guard not noexcept");
#endif

#ifdef test_2
    static_assert(noexcept(make_scope_guard(std::declval<void(*)()noexcept>())
                           .~scope_guard()),
                  "scope_guard dtor not noexcept");
#endif

    /**
     * Test nothrow character of make_scope_guard for different value categories
     * of a type with a throwing destructor
     */
    void test_throwing_dtor_throw_spec_good()
    {
#ifdef test_3
      potentially_throwing_dtor x;
      static_assert(noexcept(make_scope_guard(x)),
                    "make_scope_guard not declared noexcept when instanced "
                    "with an lvalue object whose dtor throws (should deduce "
                    "reference and avoid destruction entirely)");
#endif
#ifdef test_4
      potentially_throwing_dtor x;
      auto& r = x;
      static_assert(noexcept(make_scope_guard(r)),
                    "make_scope_guard not declared noexcept when instanced "
                    "with an lvalue reference to an object whose dtor throws "
                    "(should deduce reference and avoid destruction entirely)");
#endif
#ifdef test_5
      potentially_throwing_dtor x;
      const auto& cr = x;
      static_assert(noexcept(make_scope_guard(cr)),
                    "make_scope_guard not declared noexcept when instanced "
                    "with an lvalue reference to a const object whose dtor "
                    "throws (should deduce reference and avoid destruction "
                    "entirely)");
#endif
    }

    /**
     * Test nothrow character of make_scope_guard for different value categories
     * of a type with a throwing copy constructor
     */
    void test_throwing_copy_throw_spec()
    {
#ifdef test_6
      static_assert(noexcept(make_scope_guard(throwing_copy{})),
                    "make_scope_guard not declared noexcept when instanced "
                    "with an rvalue object whose copy ctor throws");
#endif
#ifdef test_7
      throwing_copy x;
      static_assert(noexcept(make_scope_guard(x)),
                    "make_scope_guard not declared noexcept when instanced "
                    "with an lvalue object whose copy ctor throws (should "
                    "deduce reference and avoid copy entirely)");
#endif
#ifdef test_8
      throwing_copy x;
      static_assert(noexcept(make_scope_guard(std::move(x))),
                    "make_scope_guard not declared noexcept when instanced "
                    "with an rvalue reference to an object whose copy ctor "
                    "throws");
#endif
#ifdef test_9
      throwing_copy x;
      auto& r = x;
      static_assert(noexcept(make_scope_guard(r)),
                    "make_scope_guard not declared noexcept when instanced "
                    "with an lvalue reference to an object whose copy ctor "
                    "throws (should deduce reference and avoid copy entirely)");
#endif
#ifdef test_10
      throwing_copy x;
      const auto& cr = x;
      static_assert(noexcept(make_scope_guard(cr)),
                    "make_scope_guard not declared noexcept when instanced "
                    "with an lvalue reference to a const object whose copy "
                    "ctor throws (should deduce reference and avoid copy "
                    "entirely)");
#endif
    }

    /**
     * Test nothrow character of make_scope_guard for different value categories
     * of a type with a throwing move constructor
     */
    void test_throwing_move_throw_spec()
    {
#ifdef test_11
      static_assert(!noexcept(make_scope_guard(throwing_move{})),
                    "make_scope_guard wrongly declared noexcept when instanced "
                    "with an rvalue object whose move ctor throws");
#endif
#ifdef test_12
      throwing_move x;
      static_assert(noexcept(make_scope_guard(x)),
                    "make_scope_guard not declared noexcept when instanced "
                    "with an lvalue object whose move ctor throws");
#endif
#ifdef test_13
      throwing_move x;
      static_assert(!noexcept(make_scope_guard(std::move(x))),
                    "make_scope_guard wrongly declared noexcept when instanced "
                    "with an rvalue reference to an object whose move ctor "
                    "throws");
#endif
#ifdef test_14
      throwing_move x;
      auto& r = x;
      static_assert(noexcept(make_scope_guard(r)),
                    "make_scope_guard not declared noexcept when instanced "
                    "with an lvalue reference to an object whose move ctor "
                    "throws");
#endif
#ifdef test_15
      throwing_move x;
      const auto& cr = x;
      static_assert(noexcept(make_scope_guard(cr)),
                    "make_scope_guard not declared noexcept when instanced "
                    "with an lvalue reference to a const object whose move "
                    "ctor throws");
#endif
    }

    /**
     * Test nothrow character of make_scope_guard for different value categories
     * of a type with a throwing copy constructor
     */
    void test_nomove_throwing_copy_throw_spec()
    {
#ifdef test_16
      static_assert(!noexcept(make_scope_guard(nomove_throwing_copy{})),
                    "make_scope_guard wrongly declared noexcept when instanced "
                    "with an rvalue object whose copy ctor throws and without "
                    "a move ctor");
#endif
#ifdef test_17
      nomove_throwing_copy x;
      static_assert(noexcept(make_scope_guard(x)),
                    "make_scope_guard not declared noexcept when instanced "
                    "with an lvalue object whose copy ctor throws and without "
                    "a move ctor (should deduce reference and avoid copy "
                    "entirely)");
#endif
#ifdef test_18
      nomove_throwing_copy x;
      static_assert(!noexcept(make_scope_guard(std::move(x))),
                    "make_scope_guard wrongly declared noexcept when instanced "
                    "with an rvalue reference to an object whose copy ctor "
                    "throws and without a move ctor");
#endif
#ifdef test_19
      nomove_throwing_copy x;
      auto& r = x;
      static_assert(noexcept(make_scope_guard(r)),
                    "make_scope_guard not declared noexcept when instanced "
                    "with an lvalue reference to an object whose copy ctor "
                    "throws (should deduce reference and avoid copy entirely)");
#endif
#ifdef test_20
      nomove_throwing_copy x;
      const auto& cr = x;
      static_assert(noexcept(make_scope_guard(cr)),
                    "make_scope_guard not declared noexcept when instanced "
                    "with an lvalue reference to a const object whose copy "
                    "ctor throws (should deduce reference and avoid copy "
                    "entirely)");
#endif
    }

    /**
     * Test nothrow character of make_scope_guard for different value categories
     * of a type with a non-throwing constructors and destructor
     */
    void test_nothrow_throw_spec()
    {
#ifdef test_21
      static_assert(noexcept(make_scope_guard(nothrow{})),
                    "make_scope_guard not declared noexcept when instanced "
                    "with an rvalue object whose ctors and dtor do not throw");
#endif
#ifdef test_22
      nothrow x;
      static_assert(noexcept(make_scope_guard(x)),
                    "make_scope_guard not declared noexcept when instanced "
                    "with an lvalue object whose ctors and dtor do not throw");
#endif
#ifdef test_23
      nothrow x;
      static_assert(noexcept(make_scope_guard(std::move(x))),
                    "make_scope_guard not declared noexcept when instanced "
                    "with an rvalue reference to an object whose ctors and "
                    "dtor do not throw");
#endif
#ifdef test_24
      nothrow x;
      auto& r = x;
      static_assert(noexcept(make_scope_guard(r)),
                    "make_scope_guard not declared noexcept when instanced "
                    "with an lvalue reference to an object whose ctors and "
                    "dtor do not throw");
#endif
#ifdef test_25
      nothrow x;
      const auto& cr = x;
      static_assert(noexcept(make_scope_guard(cr)),
                    "make_scope_guard not declared noexcept when instanced "
                    "with an lvalue reference to a const object whose ctors "
                    "dtor do not throw");
#endif
    }

  /**
   * Test compilation successes with wrong usage of non-copyable and non-movable
   * objects to create scope_guards
   */
  void test_noncopyable_nonmovable_good()
  {
#ifdef test_26
    nocopy_nomove ncnm{};
    std::ignore = make_scope_guard(ncnm);
#endif
#ifdef test_27
    nocopy_nomove ncnm{};
    auto& ncnmr = ncnm;
    std::ignore = make_scope_guard(ncnmr);
#endif
#ifdef test_28
    nocopy_nomove ncnm{};
    const auto& ncnmcr = ncnm;
    std::ignore = make_scope_guard(ncnmcr);
#endif
  }

  /**
   * Test dismiss is noexcept
   */
  void test_dismiss_is_noexcept()
  {
#ifdef test_29
    static_assert(noexcept(make_scope_guard(non_throwing).dismiss()),
                  "scope_guard::dismiss not noexcept");
#endif
#ifdef test_30
    static_assert(noexcept(make_scope_guard(non_throwing_lambda).dismiss()),
                  "scope_guard::dismiss not noexcept");
#endif
#ifdef test_31
    static_assert(noexcept(make_scope_guard(non_throwing_functor).dismiss()),
                  "scope_guard::dismiss not noexcept");
#endif
  }

  /**
   * Test scope_guard can always be created with noexcept marked callables
   */
  void test_noexcept_good()
  {
#ifdef test_32
    std::ignore = make_scope_guard(non_throwing);
#endif
#ifdef test_33
    std::ignore = make_scope_guard(non_throwing_lambda);
#endif
#ifdef test_34
    std::ignore = make_scope_guard(non_throwing_functor);
#endif
  }

  /* --- tests that fail iff nothrow_invocable is required --- */

  /**
   * Highlight that scope_guard should not be created with throwing callables,
   * under penalty of an immediate std::terminate call. Test that compilation
   * fails on such an attempt when noexcept is required.
   */
  void test_noexcept_bad()
  {
#ifdef test_35
    std::ignore = make_scope_guard(throwing);
#endif
#ifdef test_36
    std::ignore = make_scope_guard(throwing_stdfun);
#endif
#ifdef test_37
    std::ignore = make_scope_guard(throwing_lambda);
#endif
#ifdef test_38
    std::ignore = make_scope_guard(throwing_bound);
#endif
#ifdef test_39
    std::ignore = make_scope_guard(throwing_functor);
#endif
  }

  /**
   * Highlight the importance of declaring scope_guard callables noexcept
   * (when they do not throw). Test that compilation fails on attempts to the
   * contrary when noexcept is required.
   */
  void test_noexcept_fixable()
  {
#ifdef test_40
    std::ignore = make_scope_guard(meh);
#endif
#ifdef test_41
    std::ignore = make_scope_guard(meh_stdfun);
#endif
#ifdef test_42
    std::ignore = make_scope_guard(meh_lambda);
#endif
#ifdef test_43
    std::ignore = make_scope_guard(meh_bound);
#endif
#ifdef test_44
    std::ignore = make_scope_guard(meh_functor);
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
#ifdef test_45
    std::ignore = make_scope_guard(non_throwing_stdfun);
#endif
#ifdef test_46
    std::ignore = make_scope_guard(non_throwing_bound);
#endif
  }

  void test_dismiss_is_noexcept_even_if_bad_callable()
  {
#ifdef test_47
    static_assert(noexcept(make_scope_guard(throwing).dismiss()),
                  "scope_guard::dismiss not noexcept");
#endif
#ifdef test_48
    static_assert(noexcept(make_scope_guard(throwing_stdfun).dismiss()),
                  "scope_guard::dismiss not noexcept");
#endif
#ifdef test_49
    static_assert(noexcept(make_scope_guard(throwing_lambda).dismiss()),
                  "scope_guard::dismiss not noexcept");
#endif
#ifdef test_50
    static_assert(noexcept(make_scope_guard(throwing_bound).dismiss()),
                  "scope_guard::dismiss not noexcept");
#endif
#ifdef test_51
    static_assert(noexcept(make_scope_guard(throwing_functor).dismiss()),
                  "scope_guard::dismiss not noexcept");
#endif
  }


  /* --- tests that always fail --- */

  void test_throwing_dtor_throw_spec_bad()
  {
#ifdef test_52
    std::ignore = make_scope_guard(potentially_throwing_dtor{});
#endif
#ifdef test_53
    potentially_throwing_dtor x;
    std::ignore = make_scope_guard(std::move(x));
#endif
  }

  /**
   * Test that compilation fails when trying to copy-construct a scope_guard
   */
  void test_disallowed_copy_construction()
  {
    const auto guard1 = make_scope_guard(non_throwing);
#ifdef test_54
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
#ifdef test_55
    guard2 = guard1;
#endif
  }

  /**
   * Test that compilation fails when trying to move-assign a scope_guard
   */
  void test_disallowed_move_assignment()
  {
    auto guard = make_scope_guard(non_throwing);
#ifdef test_56
    guard = make_scope_guard(non_throwing_lambda);
#endif
  }

  /**
   * Test that compilation fails when trying to use a returning function to
   * create a scope_guard
   */
  void test_disallowed_return()
  {
#ifdef test_57
    std::ignore = make_scope_guard(returning);
#endif
#ifdef test_58
    std::ignore = make_scope_guard(returning_stdfun);
#endif
#ifdef test_59
    std::ignore = make_scope_guard(returning_lambda);
#endif
#ifdef test_60
    std::ignore = make_scope_guard(returning_bound);
#endif
#ifdef test_61
    std::ignore = make_scope_guard(returning_functor);
#endif
  }

  /**
   * Test compilation failures with wrong usage of non-copyable and non-movable
   * objects to create scope_guards
   */
  void test_noncopyable_nonmovable_bad()
  {
#ifdef test_62
    std::ignore = make_scope_guard(nocopy_nomove{});
#endif
#ifdef test_63
    nocopy_nomove ncnm{};
    std::ignore = make_scope_guard(std::move(ncnm));
#endif
  }

  /**
   * Test compilation failures when trying to create a scope guard from a
   * callback directly through its constructor
   */
  void test_direct_construction_forbidden()
  {
#ifdef test_64
    detail::scope_guard{non_throwing};
#endif
#ifdef test_65
    auto x = detail::scope_guard(non_throwing);
#endif
#ifdef test_66
    auto x = detail::scope_guard<void(&)()noexcept>{non_throwing};
#endif
#ifdef test_67
    auto lambda = []() noexcept {};
    detail::scope_guard<decltype(lambda)>{std::move(lambda)};
#endif
  }

  /**
   * Test compilation failures when trying to use modifying operations on a
   * const scope guard
   */
  void test_const_guard_cannot_do_everything()
  {
    const auto guard = make_scope_guard(non_throwing);
#ifdef test_68
    guard.dismiss();
#endif
#ifdef test_69
    auto another = std::move(guard);
#endif
  }

  /**
   * Test compilation failures when trying to inherit from scope_guard
   */
#ifdef test_70
  struct concrete_specialized_guard
    : detail::scope_guard<void(*)()noexcept>
  {};
#endif
#ifdef test_71
  template<typename T>
  struct specialized_guard : detail::scope_guard<T>
  {
    explicit specialized_guard(T&& t)
      : detail::scope_guard<T>{std::forward<T>(t)}
    {}
  };

  template<typename T>
  specialized_guard<T> make_specialized_guard(T&& t)
  {
    return specialized_guard<T>{std::forward<T>(t)};
  }

  auto special = make_specialized_guard([]()noexcept{});
#endif

}

int main()
{
  test_throwing_dtor_throw_spec_good();
  test_throwing_copy_throw_spec();
  test_throwing_move_throw_spec();
  test_nomove_throwing_copy_throw_spec();
  test_nothrow_throw_spec();
  test_noncopyable_nonmovable_good();
  test_dismiss_is_noexcept();
  test_noexcept_good();

  test_noexcept_bad(); // this results in a call to std::terminate
  test_noexcept_fixable();
  test_noexcept_unfortunate();
  test_dismiss_is_noexcept_even_if_bad_callable();

  test_throwing_dtor_throw_spec_bad();
  test_disallowed_copy_construction();
  test_disallowed_copy_assignment();
  test_disallowed_move_assignment();
  test_disallowed_return();
  test_noncopyable_nonmovable_bad();
  test_direct_construction_forbidden();

  return 0;
}
