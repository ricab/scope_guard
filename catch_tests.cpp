/*
 *  Created on: 13/02/2018
 *      Author: ricab
 */

#include "scope_guard.hpp"

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch2/catch.hpp"

#include <functional>
#include <list>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

using namespace sg;

////////////////////////////////////////////////////////////////////////////////
namespace
{
  auto count = 0u;
  void incc(unsigned& c) noexcept { ++c; }
  void inc() noexcept { incc(count); }
  void resetc(unsigned& c) noexcept { c = 0u; }
  void reset() noexcept { resetc(count); }
} // namespace

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Showing that direct constructor is not desirable.")
{
  using detail::scope_guard;

  // even if the constructor was public, it would be cumbersome...

/*  scope_guard{inc};  ... error: does not deduce template args (at least
                       until C++17); it also does not accept everything... */

/*  scope_guard<decltype(inc)>{inc}; ... error: cannot instantiate data field
                                     with function type... */

/*  scope_guard<void(&)()noexcept>{
      static_cast<void(&)()noexcept>(inc)}; ... could use ref cast... */

/*  auto& inc_ref = inc;
    scope_guard<decltype(inc_ref)>{inc_ref}; ... or actual ref... */

/*  scope_guard<decltype(inc)&&>{std::move(inc)}; ... or even rvalue ref, which
                                                  with functions is treated
                                                  just like an lvalue... */

  std::ignore = make_scope_guard(inc); // ... but the BEST is really to use the
                                       // make function
}

/* --- Plain functions, lvalues, rvalues, plain references, consts --- */

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A plain function can be used to create a scope_guard.")
{
  std::ignore = make_scope_guard(inc);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A plain-function-based scope_guard executes the function exactly "
          "once when leaving scope.")
{
  reset();

  {
    const auto guard = make_scope_guard(inc);
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}


////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A non-const, plain-function-based scope_guard can be dismissed.")
{
  make_scope_guard(inc).dismiss();
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed plain-function-based scope_guard does not execute its "
          "callback at all.")
{
  reset();

  {
    auto guard = make_scope_guard(inc);
    REQUIRE_FALSE(count);

    guard.dismiss();
    REQUIRE_FALSE(count);
  }

  REQUIRE_FALSE(count);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An lvalue reference to a plain function can be used to create a "
          "scope_guard.")
{
  auto& inc_ref = inc;
  std::ignore = make_scope_guard(inc_ref);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An lvalue-reference-to-plain-function-based scope_guard executes "
          "the function exactly once when leaving scope.")
{
  reset();

  {
    auto& inc_ref = inc;
    const auto guard = make_scope_guard(inc_ref);
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}


////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A non-const, lvalue-reference-to-plain-function-based scope_guard "
          "can be dismissed.")
{
  auto& inc_ref = inc;
  make_scope_guard(inc_ref).dismiss();
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed lvalue-reference-to-plain-function-based scope_guard "
          "does not execute its callback at all.")
{
  reset();

  {
    auto& inc_ref = inc;
    auto guard = make_scope_guard(inc_ref);
    REQUIRE_FALSE(count);

    guard.dismiss();
    REQUIRE_FALSE(count);
  }

  REQUIRE_FALSE(count);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An lvalue const reference to a plain function can be used to create "
          "a scope_guard.")
{
  const auto& inc_ref = inc;
  std::ignore = make_scope_guard(inc_ref);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An lvalue-const-reference-to-plain-function-based scope_guard "
          "executes the function exactly once when leaving scope.")
{
  reset();

  {
    const auto& inc_ref = inc;
    const auto guard = make_scope_guard(inc_ref);
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A non-const, lvalue-const-reference-to-plain-function-based "
          "scope_guard can be dismissed.")
{
  const auto& inc_ref = inc;
  make_scope_guard(inc_ref).dismiss();
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed lvalue-const-reference-to-plain-function-based "
          "scope_guard does not execute its callback at all.")
{
  reset();

  {
    const auto& inc_ref = inc;
    auto guard = make_scope_guard(inc_ref);
    REQUIRE_FALSE(count);

    guard.dismiss();
    REQUIRE_FALSE(count);
  }

  REQUIRE_FALSE(count);
}

/* Rvalue references to function should not work with make_scope_guard when type
deduction is employed: non-ref function type would be deduced, which cannot be a
data member. This is the case in MSVC, which is why the tests below are disabled
for that compiler. Clang and GCC accept it though, deducing rvalue reference
type, which is treated as lvalue reference. */

#ifndef _MSC_VER
////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An rvalue reference to a plain function can be used to create a "
          "scope_guard.")
{
  std::ignore = make_scope_guard(std::move(inc)); // rvalue ref to function treated as lvalue
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An rvalue-reference-to-plain-function-based scope_guard executes "
          "the function exactly once when leaving scope.")
{
  reset();

  {
    const auto guard = make_scope_guard(std::move(inc));
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A non-const, rvalue-reference-to-plain-function-based scope_guard "
          "can be dismissed.")
{
  make_scope_guard(std::move(inc)).dismiss();
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed rvalue-reference-to-plain-function-based scope_guard "
          "does not execute its callback at all.")
{
  reset();

  {
    auto guard = make_scope_guard(std::move(inc));
    REQUIRE_FALSE(count);

    guard.dismiss();
    REQUIRE_FALSE(count);
  }

  REQUIRE_FALSE(count);
}
#endif

/* --- std::ref and std::cref --- */

#ifndef SG_REQUIRE_NOEXCEPT
////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A reference wrapper to a plain function can be used to create a "
          "scope_guard.")
{
  std::ignore = make_scope_guard(std::ref(inc));
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A reference-wrapper-to-plain-function-based scope_guard executes "
          "the function exactly once when leaving scope.")
{
  reset();

  {
    const auto guard = make_scope_guard(std::ref(inc));
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A non-const, reference-wrapper-to-plain-function-based scope_guard "
          "can be dismissed.")
{
  make_scope_guard(std::ref(inc)).dismiss();
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed reference-wrapper-to-plain-function-based scope_guard "
          "does not execute its callback at all.")
{
  reset();

  {
    auto guard = make_scope_guard(std::ref(inc));
    REQUIRE_FALSE(count);

    guard.dismiss();
    REQUIRE_FALSE(count);
  }

  REQUIRE_FALSE(count);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A const reference wrapper to a plain function can be used to create "
          "a scope_guard.")
{
  std::ignore = make_scope_guard(std::cref(inc));
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A const-reference-wrapper-to-plain-function-based scope_guard "
          "executes the function exactly once when leaving scope.")
{
  reset();

  {
    const auto guard = make_scope_guard(std::cref(inc));
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A non-const, const-reference-wrapper-to-plain-function-based "
          "scope_guard can be dismissed.")
{
  make_scope_guard(std::cref(inc)).dismiss();
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed const-reference-wrapper-to-plain-function-based "
          "scope_guard does not execute its callback at all.")
{
  reset();

  {
    auto guard = make_scope_guard(std::cref(inc));
    REQUIRE_FALSE(count);

    guard.dismiss();
    REQUIRE_FALSE(count);
  }

  REQUIRE_FALSE(count);
}
#endif

/* --- function pointers lvalues/rvalues and references thereof --- */

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An lvalue plain function pointer can be used to create a "
          "scope_guard.")
{
  const auto fp = &inc;
  std::ignore = make_scope_guard(fp);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An lvalue-plain-function-pointer-based scope_guard executes the "
          "function exactly once when leaving scope.")
{
  reset();

  {
    const auto fp = &inc;
    const auto guard = make_scope_guard(fp);
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A non-const, lvalue-plain-function-pointer-based scope_guard can be "
          "dismissed.")
{
  const auto fp = &inc;
  make_scope_guard(fp).dismiss();
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed lvalue-plain-function-pointer-based scope_guard does "
          "not execute its callback at all.")
{
  reset();

  {
    const auto fp = &inc;
    auto guard = make_scope_guard(fp);
    REQUIRE_FALSE(count);

    guard.dismiss();
    REQUIRE_FALSE(count);
  }

  REQUIRE_FALSE(count);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An rvalue plain function pointer can be used to create a "
          "scope_guard.")
{
  std::ignore = make_scope_guard(&inc);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An rvalue-plain-function-pointer-based scope_guard executes the "
          "function exactly once when leaving scope.")
{
  reset();

  {
    const auto guard = make_scope_guard(&inc);
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A non-const, rvalue-plain-function-pointer-based scope_guard can be "
          "dismissed.")
{
  make_scope_guard(&inc).dismiss();
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed rvalue-plain-function-pointer-based scope_guard does "
          "not execute its callback at all.")
{
  reset();

  {
    auto guard = make_scope_guard(&inc);
    REQUIRE_FALSE(count);

    guard.dismiss();
    REQUIRE_FALSE(count);
  }

  REQUIRE_FALSE(count);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An lvalue reference to a plain function pointer can be used to "
          "create a scope_guard.")
{
  const auto fp = &inc;
  const auto& fp_ref = fp;
  std::ignore = make_scope_guard(fp_ref);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A plain-function-pointer-lvalue-reference-based scope_guard "
          "executes the function exactly once when leaving scope.")
{
  reset();

  {
    const auto fp = &inc;
    const auto& fp_ref = fp;
    const auto guard = make_scope_guard(fp_ref);
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A non-const, plain-function-pointer-lvalue-reference-based "
          "scope_guard can be dismissed.")
{
  const auto fp = &inc;
  const auto& fp_ref = fp;
  make_scope_guard(fp_ref).dismiss();
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed plain-function-pointer-lvalue-reference-based "
          "scope_guard does not execute its callback at all.")
{
  reset();

  {
    const auto fp = &inc;
    const auto& fp_ref = fp;
    auto guard = make_scope_guard(fp_ref);
    REQUIRE_FALSE(count);

    guard.dismiss();
    REQUIRE_FALSE(count);
  }

  REQUIRE_FALSE(count);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An rvalue reference to a plain function pointer can be used to "
          "create a scope_guard.")
{
  const auto fp = &inc;
  std::ignore = make_scope_guard(std::move(fp));
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A plain-function-pointer-rvalue-reference-based scope_guard "
          "executes the function exactly once when leaving scope.")
{
  reset();

  {
    const auto fp = &inc;
    const auto guard = make_scope_guard(std::move(fp));
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A non-const, plain-function-pointer-rvalue-reference-based "
          "scope_guard can be dismissed.")
{
  const auto fp = &inc;
  make_scope_guard(std::move(fp)).dismiss();
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed plain-function-pointer-rvalue-reference-based "
          "scope_guard does not execute its callback at all.")
{
  reset();

  {
    const auto fp = &inc;
    auto guard = make_scope_guard(std::move(fp));
    REQUIRE_FALSE(count);

    guard.dismiss();
    REQUIRE_FALSE(count);
  }

  REQUIRE_FALSE(count);
}

/* --- std::function lvalues/rvalues and references thereof --- */

////////////////////////////////////////////////////////////////////////////////
namespace
{
  template<typename Fun>
  struct remove_noexcept
  {
    using type = Fun;
  };

  template<typename Ret, typename... Args>
  struct remove_noexcept<Ret(Args...) noexcept>
  {
    using type = Ret(Args...);
  };

  template<typename Fun>
  using remove_noexcept_t = typename remove_noexcept<Fun>::type;

  template<typename Fun>
  std::function<remove_noexcept_t<Fun>>
  make_std_function(Fun& f) // ref prevents decay to pointer (no move needed)
  {
    return std::function<
      remove_noexcept_t<typename std::remove_reference<Fun>::type>>{f}; /*
    unfortunately in C++17 std::function does not accept a noexcept target type
    (results in incomplete type - at least in gcc and clang) */
  }
} // namespace

#ifndef SG_REQUIRE_NOEXCEPT
////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An lvalue std::function that wraps a regular function can be used "
          "to create a scope_guard.")
{
  const auto stdf = make_std_function(inc);
  std::ignore = make_scope_guard(stdf);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard that is created with a "
          "regular-function-wrapping lvalue std::function executes that "
          "std::function exactly once when leaving scope.")
{
  reset();

  {
    const auto stdf = make_std_function(inc);
    const auto guard = make_scope_guard(stdf);
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A non-const scope_guard that is created with a "
          "regular-function-wrapping lvalue std::function can be dismissed.")
{
  const auto stdf = make_std_function(inc);
  make_scope_guard(stdf).dismiss();
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed scope_guard that was created with a "
          "regular-function-wrapping lvalue std::function does not execute its "
          "callback at all.")
{
  reset();

  {
    const auto stdf = make_std_function(inc);
    auto guard = make_scope_guard(stdf);
    REQUIRE_FALSE(count);

    guard.dismiss();
    REQUIRE_FALSE(count);
  }

  REQUIRE_FALSE(count);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An rvalue std::function that wraps a regular function can be used "
          "to create a scope_guard.")
{
  std::ignore = make_scope_guard(make_std_function(inc));
  std::ignore = make_scope_guard(std::function<void()>{inc});
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard that is created with an "
          "regular-function-wrapping rvalue std::function executes that "
          "std::function exactly once when leaving scope.")
{
  reset();

  {
    const auto guard = make_scope_guard(make_std_function(inc));
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A non-const scope_guard that is created with an "
          "regular-function-wrapping rvalue std::function can be dismissed.")
{
  make_scope_guard(make_std_function(inc)).dismiss();
  make_scope_guard(std::function<void()>{inc}).dismiss();
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed scope_guard that was created with an "
          "regular-function-wrapping rvalue std::function does not execute its "
          "callback at all.")
{
  reset();

  {
    auto guard = make_scope_guard(make_std_function(inc));
    REQUIRE_FALSE(count);

    guard.dismiss();
    REQUIRE_FALSE(count);
  }

  REQUIRE_FALSE(count);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An lvalue reference to a std::function that wraps a regular "
          "function can be used to create a scope_guard.")
{
  const auto stdf = make_std_function(inc);
  const auto& stdf_ref = stdf;
  std::ignore = make_scope_guard(stdf_ref);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard that is created with a "
          "regular-function-wrapping std::function lvalue reference "
          "executes that std::function exactly once when leaving scope.")
{
  reset();

  {
    const auto stdf = make_std_function(inc);
    const auto& stdf_ref = stdf;
    const auto guard = make_scope_guard(stdf_ref);
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A non-const scope_guard that was created with a "
          "regular-function-wrapping std::function lvalue reference can be "
          "dismissed.")
{
  const auto stdf = make_std_function(inc);
  const auto& stdf_ref = stdf;
  make_scope_guard(stdf_ref).dismiss();
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed scope_guard that was created with a "
          "regular-function-wrapping std::function lvalue reference does not "
          "execute its callback at all.")
{
  reset();

  {
    const auto stdf = make_std_function(inc);
    const auto& stdf_ref = stdf;
    auto guard = make_scope_guard(stdf_ref);
    REQUIRE_FALSE(count);

    guard.dismiss();
    REQUIRE_FALSE(count);
  }

  REQUIRE_FALSE(count);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An rvalue reference to a std::function that wraps a regular "
          "function can be used to create a scope_guard.")
{
  const auto stdf = make_std_function(inc);
  std::ignore = make_scope_guard(std::move(stdf));
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard that is created with an "
          "regular-function-wrapping std::function rvalue reference"
          "executes that std::function exactly once when leaving scope.")
{
  reset();

  {
    const auto stdf = make_std_function(inc);
    const auto guard = make_scope_guard(std::move(stdf));
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A non-const scope_guard that is created with a "
          "regular-function-wrapping std::function rvalue reference can be "
          "dismissed.")
{
  const auto stdf = make_std_function(inc);
  make_scope_guard(std::move(stdf)).dismiss();
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed scope_guard that was created with a "
          "regular-function-wrapping std::function rvalue reference does not "
          "execute its callback at all.")
{
  reset();

  {
    const auto stdf = make_std_function(inc);
    auto guard = make_scope_guard(std::move(stdf));
    REQUIRE_FALSE(count);

    guard.dismiss();
    REQUIRE_FALSE(count);
  }

  REQUIRE_FALSE(count);
}
#endif

/* --- lambdas --- */

////////////////////////////////////////////////////////////////////////////////
namespace
{
  auto lambda_no_capture_count = 0u;
} // namespace

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A lambda function with no capture can be used to create a "
          "scope_guard.")
{
  std::ignore = make_scope_guard([]()noexcept{});
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A no-capture-lambda-based scope_guard executes the lambda exactly "
          "once when leaving scope.")
{
  {
    resetc(lambda_no_capture_count);
    const auto guard = make_scope_guard([]() noexcept
                                        { incc(lambda_no_capture_count); });
    REQUIRE_FALSE(lambda_no_capture_count);
  }

  REQUIRE(lambda_no_capture_count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A non-const, no-capture-lambda-based scope_guard can be dismissed.")
{
  make_scope_guard([]()noexcept{}).dismiss();
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed no-capture-lambda-based scope_guard does not execute "
          "its callback at all.")
{
  {
    resetc(lambda_no_capture_count);
    auto guard = make_scope_guard([]() noexcept
                                  { incc(lambda_no_capture_count); });
    REQUIRE_FALSE(lambda_no_capture_count);

    guard.dismiss();
    REQUIRE_FALSE(lambda_no_capture_count);
  }

  REQUIRE_FALSE(lambda_no_capture_count);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A lambda function with capture can be used to create a scope_guard.")
{
  auto f = 0.0f;
  const auto i = -1;
  std::ignore = make_scope_guard([&f, i]() noexcept
                                 { f = static_cast<float>(*&i); });
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A capturing-lambda-based scope_guard executes the lambda when "
          "leaving scope.")
{
  auto lambda_count = 0u;

  {
    const auto guard = make_scope_guard([&lambda_count]() noexcept
                                        { incc(lambda_count); });
    REQUIRE_FALSE(lambda_count);
  }

  REQUIRE(lambda_count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A non-const, capturing-lambda-based scope_guard can be dismissed.")
{
  auto i = -1;
  const auto f = 0.0f;
  make_scope_guard([f, &i]()noexcept{ i = static_cast<int>(*&f); }).dismiss();
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed capturing-lambda-based scope_guard does not execute its "
          "callback at all.")
{
  auto lambda_count = 0u;

  {
    auto guard = make_scope_guard([&lambda_count]() noexcept
                                  { incc(lambda_count); });
    REQUIRE_FALSE(lambda_count);

    guard.dismiss();
    REQUIRE_FALSE(lambda_count);
  }

  REQUIRE_FALSE(lambda_count);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A const lambda function with capture can be used to create a "
          "scope_guard.")
{
  auto f = 0.0f;
  const auto i = -1;
  const auto lambda = [&f, i]() noexcept { f = static_cast<float>(*&i); };
  std::ignore = make_scope_guard(lambda);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A const-capturing-lambda-based scope_guard executes the lambda when "
          "leaving scope.")
{
  auto lambda_count = 0u;

  {
    const auto lambda = [&lambda_count]() noexcept { incc(lambda_count); };
    const auto guard = make_scope_guard(lambda);
    REQUIRE_FALSE(lambda_count);
  }

  REQUIRE(lambda_count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A non-const, const-capturing-lambda-based scope_guard can be "
          "dismissed.")
{
  auto f = 0.0f;
  const auto i = -1;
  const auto lambda = [&f, i]() noexcept { f = static_cast<float>(*&i); };
  make_scope_guard(lambda).dismiss();
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed const-capturing-lambda-based scope_guard does not "
          "execute its callback at all.")
{
  auto lambda_count = 0u;

  {
    const auto lambda = [&lambda_count]() noexcept { incc(lambda_count); };
    auto guard = make_scope_guard(lambda);
    REQUIRE_FALSE(lambda_count);

    guard.dismiss();
    REQUIRE_FALSE(lambda_count);
  }

  REQUIRE_FALSE(lambda_count);
}

/* --- mixes of plain function, std::function, and lambda indirections --- */

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard created with a regular-function-calling lambda, "
          "calls the lambda exactly once when leaving scope, which in turn "
          "calls the regular function.")
{
  reset();
  auto lambda_count = 0u;

  {
    const auto guard = make_scope_guard([&lambda_count]() noexcept
                                        { inc(); incc(lambda_count); });
    REQUIRE_FALSE(count);
    REQUIRE_FALSE(lambda_count);
  }

  REQUIRE(count == lambda_count);
  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed scope_guard that was created with a "
          "regular-function-calling lambda, does not execute its callback at "
          "all.")
{
  reset();
  auto lambda_count = 0u;

  {
    auto guard = make_scope_guard([&lambda_count]() noexcept
                                  { inc(); incc(lambda_count); });
    REQUIRE_FALSE(count);
    REQUIRE_FALSE(lambda_count);

    guard.dismiss();
    REQUIRE_FALSE(count);
    REQUIRE_FALSE(lambda_count);
  }

  REQUIRE_FALSE(count);
  REQUIRE_FALSE(lambda_count);
}

/* only sparse dismiss tests below this point */

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A lambda function calling a std::function can be used to create a "
          "scope_guard.")
{
  std::ignore = make_scope_guard([]()noexcept{ make_std_function(inc)(); });
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard created with a std::function-calling lambda calls "
          "the lambda exactly once when leaving scope, which in turn calls the "
          "std::function.")
{
  reset();
  auto lambda_count = 0u;

  {
    const auto guard = make_scope_guard(
      [&lambda_count]() noexcept
      {
        incc(lambda_count);
        make_std_function(inc)();
      });

    REQUIRE_FALSE(count);
    REQUIRE_FALSE(lambda_count);
  }

  REQUIRE(count == lambda_count);
  REQUIRE(count == 1u);
}

#ifndef SG_REQUIRE_NOEXCEPT
////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A std::function wrapping a lambda function can be used to create a "
          "scope_guard.")
{
  std::ignore = make_scope_guard(std::function<void()>([](){}));
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard created with a lambda-wrapping std::function calls "
          "the std::function exactly once when leaving scope.")
{
  reset();

  {
    const auto guard = make_scope_guard(std::function<void()>([](){ inc(); }));
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

/* --- bound functions (std::bind) --- */

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A bound function can be used to create a scope_guard.")
{
  auto boundf_count = 0u;
  std::ignore = make_scope_guard(std::bind(incc, std::ref(boundf_count)));
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A bound-function-based scope_guard calls the bound function exactly "
          "once when leaving scope.")
{
  auto boundf_count = 0u;

  {
    const auto guard = make_scope_guard(std::bind(incc,
                                                  std::ref(boundf_count)));
    REQUIRE_FALSE(boundf_count);
  }

  REQUIRE(boundf_count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed bound-function-based scope_guard does not execute its "
          "callback at all")
{
  auto boundf_count = 0u;

  {
    auto guard = make_scope_guard(std::bind(incc, std::ref(boundf_count)));
    REQUIRE_FALSE(boundf_count);

    guard.dismiss();
    REQUIRE_FALSE(boundf_count);
  }

  REQUIRE_FALSE(boundf_count);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A bound lambda can be used to create a scope_guard.")
{
  std::ignore = make_scope_guard(std::bind([](int /*unused*/){}, 42));
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A bound-lambda-based scope_guard calls the bound lambda exactly "
          "once when leaving scope.")
{
  auto boundl_count = 0u;

  {
    const auto incc_l = [](unsigned& c){ incc(c); };
    const auto guard = make_scope_guard(std::bind(incc_l,
                                                  std::ref(boundl_count)));
    REQUIRE_FALSE(boundl_count);
  }

  REQUIRE(boundl_count == 1u);
}
#endif

/* --- custom functors --- */

////////////////////////////////////////////////////////////////////////////////
namespace
{
  struct StatelessFunctor
  {
    void operator()() const noexcept { inc(); }
  };

  struct StatefulFunctor
  {
    explicit StatefulFunctor(unsigned& c) : m_c{c} {}
    void operator()() const noexcept { incc(m_c); }

    unsigned& m_c;
  };

  struct nocopy_nomove // non-copyable and non-movable
  {
    void operator()() const noexcept { inc(); }

    nocopy_nomove() = default;
    ~nocopy_nomove() = default;

    nocopy_nomove(const nocopy_nomove&) = delete;
    nocopy_nomove& operator=(const nocopy_nomove&) = delete;
    nocopy_nomove(nocopy_nomove&&) = delete;
    nocopy_nomove& operator=(nocopy_nomove&&) = delete;
  };
} // namespace

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A stateless custom functor can be used to create a scope_guard")
{
  std::ignore = make_scope_guard(StatelessFunctor{});
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A stateless-custom-functor-based scope_guard calls the functor "
          "exactly once when leaving scope.")
{
  reset();

  {
    const auto guard = make_scope_guard(StatelessFunctor{});
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A stateful custom functor can be used to create a scope_guard")
{
  auto u = 123u;
  std::ignore = make_scope_guard(StatefulFunctor{u});
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A stateful-custom-functor-based scope_guard calls the functor "
          "exactly once when leaving scope.")
{
  auto functor_count = 0u;

  {
    const auto guard = make_scope_guard(StatefulFunctor{functor_count});
    REQUIRE_FALSE(functor_count);
  }

  REQUIRE(functor_count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A const custom functor can be used to create a scope_guard")
{
  const auto fun = StatelessFunctor{};
  std::ignore = make_scope_guard(fun);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A const-custom-functor-based scope_guard calls the functor "
          "exactly once when leaving scope.")
{
  reset();

  {
    const auto fun = StatelessFunctor{};
    const auto guard = make_scope_guard(fun);
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An lvalue reference to a noncopyable and nonmovable functor can be "
          "used to create a scope_guard")
{
  nocopy_nomove ncnm{};
  const auto& ncnm_ref = ncnm;
  std::ignore = make_scope_guard(ncnm_ref);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard created with an lvalue reference to a noncopyable and "
          "nonmovable functor calls the functor exactly once when leaving "
          "scope")
{
  reset();

  {
    nocopy_nomove ncnm{};
    const auto& ncnm_ref = ncnm;
    const auto guard = make_scope_guard(ncnm_ref);

    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed scope_guard that was created with an lvalue reference "
          "to a noncopyable and nonmovable functor does not execute its "
          "callback at all.")
{
  reset();

  {
    nocopy_nomove ncnm{};
    const auto& ncnm_ref = ncnm;
    auto guard = make_scope_guard(ncnm_ref);

    REQUIRE_FALSE(count);

    guard.dismiss();
    REQUIRE_FALSE(count);
  }

  REQUIRE_FALSE(count);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An lvalue noncopyable and nonmovable functor can be used to create "
          "a scope_guard, because it binds to an lvalue reference")
{
  nocopy_nomove ncnm{};
  std::ignore = make_scope_guard(ncnm);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard created with an lvalue noncopyable and nonmovable "
          "functor calls the functor exactly once when leaving scope")
{
  reset();

  {
    nocopy_nomove ncnm{};
    const auto guard = make_scope_guard(ncnm);

    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A const lvalue noncopyable and nonmovable functor can be used to "
          "create a scope_guard, provided its operator() is const, because it "
          "binds to a const lvalue reference")
{
  const nocopy_nomove ncnm{};
  std::ignore = make_scope_guard(ncnm);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard created with a const lvalue noncopyable and "
          "nonmovable functor calls the functor exactly once when leaving "
          "scope")
{
  reset();

  {
    const nocopy_nomove ncnm{};
    const auto guard = make_scope_guard(ncnm);

    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Redundant scope_guards do not interfere with each other - their "
          "combined post-condition holds.")
{
  reset();
  auto lambda_count = 0u;

  {
    const auto g1 = make_scope_guard([&lambda_count]() noexcept
                                     { inc(); incc(lambda_count); });
    REQUIRE_FALSE(count);
    REQUIRE_FALSE(lambda_count);
    const auto g2 = make_scope_guard([&lambda_count]() noexcept
                                     { incc(lambda_count); inc(); });
    REQUIRE_FALSE(count);
    REQUIRE_FALSE(lambda_count);
    const auto g3 = make_scope_guard(inc);
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 3u);
  REQUIRE(lambda_count == 2u);

  const auto g4 = make_scope_guard([&lambda_count]() noexcept
                                   { incc(lambda_count); inc(); });
  REQUIRE(count == 3u);
  REQUIRE(lambda_count == 2u);
}

/* --- methods --- */

////////////////////////////////////////////////////////////////////////////////
namespace
{
  struct regular_method_holder
  {
    regular_method_holder() = default;
    void regular_inc_method() noexcept { incc(m_count); }

    unsigned m_count = 0u;
  };

  struct const_method_holder
  {
    const_method_holder() = default;
    void const_inc_method() const noexcept { incc(m_count); }

    mutable unsigned m_count = 0u;
  };

  struct static_method_holder
  {
    static_method_holder() = delete;
    static void static_inc_method() noexcept { incc(ms_count); }

    static unsigned ms_count;
  };
  unsigned static_method_holder::ms_count = 0u;

  struct virtual_method_holder_pure_base
  {
    virtual ~virtual_method_holder_pure_base() = default;
    virtual void virtual_inc_method() noexcept = 0;

    unsigned m_count = 0;
  };

  struct virtual_method_holder_intermediate :
    public virtual_method_holder_pure_base
  {
    void virtual_inc_method() noexcept override
    {
      m_count += 2;
    }
  };

  struct virtual_method_holder : public virtual_method_holder_intermediate
  {
    void virtual_inc_method() noexcept override
    {
      virtual_method_holder_intermediate::virtual_inc_method();
      --m_count;
    }
  };
} // namespace

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A lambda-wrapped regular method can be used to create a scope_guard")
{
  regular_method_holder h{};
  std::ignore = make_scope_guard([&h]() noexcept { h.regular_inc_method(); });
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A lambda-wrapped-regular-method-based scope_guard executes the "
          "method exactly once when leaving scope")
{
  regular_method_holder h{};

  {
    const auto guard = make_scope_guard([&h]() noexcept
                                        { h.regular_inc_method(); });
    REQUIRE_FALSE(h.m_count);
  }

  REQUIRE(h.m_count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed lambda-wrapped-regular-method-based scope_guard does "
          "not execute its callback at all.")
{
  regular_method_holder h{};

  {
    auto guard = make_scope_guard([&h]() noexcept { h.regular_inc_method(); });
    REQUIRE_FALSE(h.m_count);

    guard.dismiss();
    REQUIRE_FALSE(h.m_count);
  }

  REQUIRE_FALSE(h.m_count);
}

#ifndef SG_REQUIRE_NOEXCEPT
////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A bound regular method can be used to create a scope_guard")
{
  regular_method_holder h{};
  std::ignore = make_scope_guard(
      std::bind(&regular_method_holder::regular_inc_method, h));
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A bound-regular-method-based scope_guard executes the method "
          "exactly once when leaving scope")
{
  regular_method_holder h{};

  {
    const auto guard = make_scope_guard(
      std::bind(&regular_method_holder::regular_inc_method, &h));
    REQUIRE_FALSE(h.m_count);
  }

  REQUIRE(h.m_count == 1u);
}
#endif

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A lambda-wrapped const method can be used to create a scope_guard")
{
  const const_method_holder h{};
  std::ignore = make_scope_guard([&h]() noexcept { h.const_inc_method(); });
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A lambda-wrapped-const-method-based scope_guard executes the "
          "method exactly once when leaving scope")
{
  const const_method_holder h{};

  {
    const auto guard = make_scope_guard([&h]() noexcept
                                        { h.const_inc_method(); });
    REQUIRE_FALSE(h.m_count);
  }

  REQUIRE(h.m_count == 1u);
}

#ifndef SG_REQUIRE_NOEXCEPT
////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A bound const method can be used to create a scope_guard")
{
  const const_method_holder h{};
  std::ignore =
      make_scope_guard(std::bind(&const_method_holder::const_inc_method, h));
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A bound-const-method-based scope_guard executes the method "
          "exactly once when leaving scope")
{
  const const_method_holder h{};

  {
    const auto guard = make_scope_guard(
      std::bind(&const_method_holder::const_inc_method, &h));
    REQUIRE_FALSE(h.m_count);
  }

  REQUIRE(h.m_count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed, bound-const-method-based scope_guard does not execute "
          "its callback at all.")
{
  const const_method_holder h{};

  {
    auto guard = make_scope_guard(
      std::bind(&const_method_holder::const_inc_method, &h));
    REQUIRE_FALSE(h.m_count);

    guard.dismiss();
    REQUIRE_FALSE(h.m_count);
  }

  REQUIRE_FALSE(h.m_count);
}
#endif

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A static method can be used to create a scope_guard")
{
  std::ignore = make_scope_guard(static_method_holder::static_inc_method);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A static-method-based scope_guard executes the static method "
          "exactly once when leaving scope")
{
  resetc(static_method_holder::ms_count);

  {
    auto guard = make_scope_guard(static_method_holder::static_inc_method);
    REQUIRE_FALSE(static_method_holder::ms_count);
  }

  REQUIRE(static_method_holder::ms_count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A lambda-wrapped virtual method can be used to create a scope_guard")
{
  virtual_method_holder h{};
  std::ignore = make_scope_guard([&h]() noexcept { h.virtual_inc_method(); });
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A lambda-wrapped-virtual-method-based scope_guard executes the "
          "virtual method exactly once when leaving scope")
{
  virtual_method_holder h{};
  virtual_method_holder_pure_base& h_base = h;

  {
    auto guard = make_scope_guard([&h_base]() noexcept
                                  { h_base.virtual_inc_method(); });

    REQUIRE_FALSE(h_base.m_count);
  }

  REQUIRE(h_base.m_count == 1u);
}

/* --- SFINAE friendliness --- */

////////////////////////////////////////////////////////////////////////////////
namespace
{
  bool goodbye_said = false;

  void fallback() noexcept
  {
    goodbye_said = false;
  }

  struct good_stream
  {
    void close() noexcept { goodbye_said = true; }
  };

  struct bad_stream
  {
    bool close() noexcept { goodbye_said = true; return true; } // returns
  };

  struct uncertain_stream
  {
    void close() { goodbye_said = true; } // not noexcept
  };

  template<typename Stream>
  struct sclosr
  {
    Stream& m_s;

    auto operator()()
    noexcept(noexcept(m_s.close()))
    -> decltype(std::declval<Stream>().close())
    {
      return m_s.close();
    }
  };

  template<typename Stream>
  auto get_closing_guard(Stream& s)
  -> decltype(make_scope_guard(sclosr<Stream>{s})) /* not using lambda because
     it can't appear in unevaluated contexts; not using bind because it does not
     preserve noexcept */
  {
    return make_scope_guard(sclosr<Stream>{s});
  }

  auto get_closing_guard(...) -> decltype(make_scope_guard(fallback))
  {
    return make_scope_guard(fallback);
  }
} // namespace

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Example usage relying on SFINAE-friendliness")
{
  {
    good_stream s;
    auto guard = get_closing_guard(s);
  }
  REQUIRE(goodbye_said);

  {
    bad_stream s;
    auto guard = get_closing_guard(s);
  }
  REQUIRE_FALSE(goodbye_said);

  {
    uncertain_stream s;
    auto guard = get_closing_guard(s);
  }
#ifdef SG_REQUIRE_NOEXCEPT
  REQUIRE_FALSE(goodbye_said);
#else
  REQUIRE(goodbye_said);
#endif
}

////////////////////////////////////////////////////////////////////////////////
namespace
{
  using tag_prefered_overload = char;

  template<typename T>
  auto sfinae_tester_impl(T&& t, tag_prefered_overload&& /*ignored*/)
  -> decltype(make_scope_guard(std::forward<T>(t)), std::declval<void>())
  {
    std::ignore = make_scope_guard(std::forward<T>(t));
  }

  template<typename T>
  void sfinae_tester_impl(T&& /*ignored*/,
                          ... /* less specific, so 2nd choice */)
  {
    std::ignore = make_scope_guard(inc);
  }

  template<typename T>
  void sfinae_tester(T&& t)
  {
    sfinae_tester_impl(std::forward<T>(t), tag_prefered_overload{}); /* the
    overload with the exact type match for the second argument is a closer
    match overall, so it will be tried first; if make_scope_guard is
    SFINAE-friendly, the other one is used as a fall-back when substitution
    fails on the former; otherwise, a compilation error is issued upon the
    substitution failure */
  }

  void noop() noexcept {}
} // namespace

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("The SFINAE testing tool creates a make_scope_guard, preferentially "
          "using the provided parameter, if possible, but discarding it "
          "otherwise - tests tool used in other tests")
{
  reset();

  sfinae_tester(noop); // does not affect count
  REQUIRE_FALSE(count);
  sfinae_tester([]() noexcept { }); // does not affect count
  REQUIRE_FALSE(count);
  sfinae_tester([]() noexcept { count = 999u; }); // affects count
  REQUIRE(count == 999u);

#ifndef SG_REQUIRE_NOEXCEPT
  sfinae_tester([]() { count = 10101u; }); // not noexcept; affects count
  REQUIRE(count == 10101u);
#endif

  reset();
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("When deducing make_scope_guard's callback type, a substitution "
          "failure caused by a non-callable can be recovered-from without a "
          "compilation error")
{
  reset();

  sfinae_tester(123); /* template deduction falls back to substitution that
                         discards the parameter (compilation would fail here if
                         scope_guard was not SFINAE-friendly) */
  REQUIRE(count == 1u);

  sfinae_tester(false); // idem
  REQUIRE(count == 2u);

  sfinae_tester("rubbish"); // idem
  REQUIRE(count == 3u);

  sfinae_tester(&count); // idem
  REQUIRE(count == 4u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("When deducing make_scope_guard's callback type, a substitution "
          "failure caused by a callable that takes arguments can be recovered "
          "from without a compilation error")
{
  reset();

  sfinae_tester(incc);
  REQUIRE(count == 1u);

  sfinae_tester(
    [](float, bool, tag_prefered_overload, virtual_method_holder) noexcept {});
  REQUIRE(count == 2u);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE("When deducing make_scope_guard's callback type, a substitution "
          "failure caused by a callable that returns non-void can be recovered "
          "from without a compilation error")
{
  reset();
  sfinae_tester([]() noexcept { return "returning"; });
  REQUIRE(count == 1u);

  sfinae_tester([]() noexcept { return true; });
  REQUIRE(count == 2u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("When deducing make_scope_guard's callback type,  substitution "
          "failure caused by a callable that is not noexcept-destructible can "
          "be recovered-from without a compilation error")
{
  reset();

  struct local
  {
    ~local() noexcept(false) {}
    void operator()() const noexcept {}
  } obj;

  sfinae_tester(std::move(obj));

  REQUIRE(count == 1u);
}

#ifdef SG_REQUIRE_NOEXCEPT
////////////////////////////////////////////////////////////////////////////////
TEST_CASE("When deducing make_scope_guard's callback type, and when noexcept "
          "is required, a substitution failure caused by a callable that is "
          "not noexcept can be recovered-from without a compilation error")
{
  reset();
  sfinae_tester([](){}); // not marked noexcept
  REQUIRE(count == 1u);
}
#endif

/* --- miscellaneous --- */

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Multiple independent scope_guards do not interfere with each "
          "other - each of their post-conditions hold.")
{
  auto a = 0u;
  auto b = 0u;
  auto c = 0u;

  {
    auto guard_a = make_scope_guard([&a]() noexcept { incc(a); });
    REQUIRE_FALSE(a);
    REQUIRE_FALSE(b);
    REQUIRE_FALSE(c);
  }
  REQUIRE(a == 1u);
  REQUIRE_FALSE(b);
  REQUIRE_FALSE(c);

  {
    auto guard_b = make_scope_guard([&b]() noexcept { incc(b); });
    auto guard_c = make_scope_guard([&c]() noexcept { incc(c); });
    REQUIRE(a == 1u);
    REQUIRE_FALSE(b);
    REQUIRE_FALSE(c);
  }
  REQUIRE(a == 1u);
  REQUIRE(b == 1u);
  REQUIRE(c == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test nested scopes")
{
  auto lvl0_count  = 0u;
  auto lvl1_count  = 0u;
  auto lvl2a_count = 0u;
  auto lvl2b_count = 0u;
  auto lvl3a_count = 0u;
  auto lvl3b_count = 0u;
  auto lvl3c_count = 0u;

  const auto lvl0_guard =
    make_scope_guard([&lvl0_count]() noexcept { incc(lvl0_count); });
  REQUIRE_FALSE(lvl0_count);

  {
    const auto lvl1_guard =
      make_scope_guard([&lvl1_count]() noexcept { incc(lvl1_count); });

    {
      const auto lvl2a_guard =
        make_scope_guard([&lvl2a_count]() noexcept { incc(lvl2a_count); });
      REQUIRE_FALSE(lvl2a_count);

      {
        const auto lvl3a_guard =
          make_scope_guard([&lvl3a_count]() noexcept { incc(lvl3a_count); });
        REQUIRE_FALSE(lvl3a_count);
      }

      REQUIRE(lvl3a_count == 1);
      REQUIRE_FALSE(lvl2a_count);
    }

    REQUIRE(lvl2a_count == 1);
    REQUIRE_FALSE(lvl1_count);
    REQUIRE_FALSE(lvl0_count);

    {
      const auto lvl2b_guard =
        make_scope_guard([&lvl2b_count]() noexcept { incc(lvl2b_count); });
      REQUIRE_FALSE(lvl2b_count);

      {
        const auto lvl3b_guard =
          make_scope_guard([&lvl3b_count]() noexcept { incc(lvl3b_count); });
        REQUIRE_FALSE(lvl3b_count);

        const auto lvl3c_guard =
          make_scope_guard([&lvl3c_count]() noexcept { incc(lvl3c_count); });
        REQUIRE_FALSE(lvl3c_count);
      }

      REQUIRE(lvl3b_count == 1);
      REQUIRE(lvl3c_count == 1);
      REQUIRE_FALSE(lvl2b_count);

    }

    REQUIRE(lvl2b_count == 1);
    REQUIRE_FALSE(lvl1_count);
    REQUIRE_FALSE(lvl0_count);

  }

  REQUIRE(lvl1_count  == 1);
  REQUIRE(lvl2a_count == 1);
  REQUIRE(lvl2b_count == 1);
  REQUIRE(lvl3a_count == 1);
  REQUIRE(lvl3b_count == 1);
  REQUIRE(lvl3c_count == 1);
  REQUIRE_FALSE(lvl0_count);

}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Dismissing a scope_guard multiple times is the same as dismissing it"
          "only once")
{
  reset();

  for(auto i = 0; i < 100; ++i)
  {
    {
      auto guard = make_scope_guard(inc);
      for(int c = 0; c <= i; ++c)
        guard.dismiss();
      REQUIRE_FALSE(count);
    }

    REQUIRE_FALSE(count);
  }
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("scope_guards execute their callback exactly once when leaving "
          "scope due to an exception")
{
  reset();
  auto countl = 0u;

  try
  {
    const auto guard = make_scope_guard(inc);
    const auto guardl = make_scope_guard([&countl]() noexcept { ++countl; });
    throw "foo";
  }
  catch(...)
  {
    REQUIRE(count == 1u);
    REQUIRE(countl == 1u);
  }
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("dismissed scope_guards do not execute their callback when leaving "
          "scope due to an exception")
{
  reset();
  auto countl = 0u;

  try
  {
    auto guard = make_scope_guard(inc);
    auto guardl = make_scope_guard([&countl]() noexcept { ++countl; });

    guard.dismiss();
    guardl.dismiss();

    throw "foo";
  }
  catch(...)
  {
    REQUIRE_FALSE(count);
    REQUIRE_FALSE(countl);
  }
}

////////////////////////////////////////////////////////////////////////////////
namespace
{
  unsigned returning(unsigned ret)
  {
    if(ret)
    {
      const auto guard = make_scope_guard(inc);
      return ret;
    }

    return 0u;
  }

  unsigned dismissing_and_returning(unsigned ret)
  {
    if(ret)
    {
      auto guard = make_scope_guard(inc);
      guard.dismiss();
      return ret;
    }

    return 0u;
  }
} // namespace

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("scope_guards execute their callback exactly once when leaving "
          "scope due to a return")
{
  reset();

  REQUIRE(123 == returning(123));
  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("dismissed scope_guards do not execute their callback when leaving "
          "scope due to a return.")
{
  reset();

  REQUIRE(123 == dismissing_and_returning(123));
  REQUIRE_FALSE(count);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("When a scope_guard is move-constructed, the original callback is "
          "executed only once, by the destination scope_guard (the source"
          "scope_guard does not call it)")
{
  reset();

  {
    auto source = make_scope_guard(inc);
    {
      const auto dest = std::move(source);
      REQUIRE_FALSE(count); // inc not executed with source move
    }
    REQUIRE(count == 1u); // inc executed with destruction of dest
  }

  REQUIRE(count == 1u); // inc not executed with destruction of source
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("When a scope_guard is move-constructed from a dismissed guard, none "
          "of the guards execute their callback")
{
  reset();

  {
    auto source = make_scope_guard(inc);
    {
      source.dismiss();
      const auto dest = std::move(source);
      REQUIRE_FALSE(count); // inc not executed with source move
    }
    REQUIRE_FALSE(count); // inc not executed with destruction of dest
  }

  REQUIRE_FALSE(count); // inc not executed with destruction of source
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Dismissing a moved-from scope_guard is valid, but has no effect.")
{
  reset();

  {
    auto source = make_scope_guard(inc);
    {
      const auto dest = std::move(source);
      source.dismiss();
      REQUIRE_FALSE(count); // inc not executed with source move or dismiss
    }
    REQUIRE(count == 1u); // inc executed with destruction of dest
  }

  REQUIRE(count == 1u); // inc not executed with destruction of source
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A dismissed moved-to scope_guard does not execute its callback")
{
  reset();

  {
    auto source = make_scope_guard(inc);
    {
      auto dest = std::move(source);
      dest.dismiss();
      REQUIRE_FALSE(count); // inc not executed with source move or dest dismiss
    }
    REQUIRE_FALSE(count); // inc not executed with destruction of dest
  }

  REQUIRE_FALSE(count); // inc not executed with destruction of source
}

#if __cplusplus >= 201402L
////////////////////////////////////////////////////////////////////////////////
TEST_CASE("When a scope_guard is move-captured, the original callback is "
          "executed only once, by the destination scope_guard (the source"
          "scope_guard does not call it)")
{
  reset();

  {
    auto source = make_scope_guard(inc);
    {
      auto lambda = [dest = std::move(source)]() noexcept { };
      REQUIRE_FALSE(count); // inc not executed with move capture
    }
    REQUIRE(count == 1u); // inc executed with destruction of capturing lambda
  }

  REQUIRE(count == 1u); // inc not executed with destruction of source
}
#endif

////////////////////////////////////////////////////////////////////////////////
namespace
{
  template<typename Callback>
  struct scope_guard_holder
  {
    explicit scope_guard_holder(detail::scope_guard<Callback>&& guard) noexcept
      : m_guard{std::move(guard)}
    {}

    detail::scope_guard<Callback> m_guard;
  };

#if __cplusplus > 201402L
  using std::make_unique;
#else
  template<typename T, typename... Args>
  inline std::unique_ptr<T> make_unique(Args&&... args)
  {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
  }
#endif
} // namespace

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard that is moved from does not call its callback when "
          "leaving scope, but the scope_guard that was moved into, does.")
{
  reset();

  {
    using Holder = scope_guard_holder<decltype(inc)&>;
    std::unique_ptr<Holder> holder = nullptr;
    {
      auto guard = make_scope_guard(inc);
      holder = make_unique<Holder>(std::move(guard));
      REQUIRE_FALSE(count);
    }

    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard that is moved into a container does not call its "
          "callback when leaving scope; the callback is called when the "
          "corresponding container element is destroyed")
{
  reset();
  std::list<decltype(make_scope_guard(inc))> l;

  {
    l.push_back(make_scope_guard(inc));
    REQUIRE_FALSE(count);
  }

  REQUIRE_FALSE(count);
  l.clear();
  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Callbacks that are used to make scope_guards can be called "
          "independently without affecting the behavior of the scope_guard")
{
  reset();


  {
    const auto lambda = []() noexcept { inc(); };
    const auto gf = make_scope_guard(inc);
    const auto gl = make_scope_guard(lambda);
    REQUIRE_FALSE(count);

    inc(); inc(); lambda();
    REQUIRE(count == 3u);
  }

  REQUIRE(count == 5u);
}

////////////////////////////////////////////////////////////////////////////////
namespace
{
  bool is_fake_done = false;
  void fake_do() noexcept { is_fake_done = true; }
  void fake_undo() noexcept { is_fake_done = false; }
} // namespace

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test custom rollback")
{
  fake_do();

  {
    auto guard = make_scope_guard(fake_undo);
    REQUIRE(is_fake_done);
  }

  REQUIRE_FALSE(is_fake_done);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test rollback due to exception")
{
  fake_do();

  try
  {
    auto guard = make_scope_guard(fake_undo);
    throw "foobar";
  }
  catch(...)
  {
    REQUIRE_FALSE(is_fake_done);
  }
}

////////////////////////////////////////////////////////////////////////////////
namespace
{
  bool fake_returning_undo(bool ret)
  {
    if(ret)
    {
      auto guard = make_scope_guard(fake_undo);
      return true;
    }

    return false;
  }
} // namespace

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test rollback due to return")
{
  fake_do();
  REQUIRE(fake_returning_undo(true));
  REQUIRE_FALSE(is_fake_done);
}
