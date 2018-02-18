#include "scope_guard.hpp"

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch/catch.hpp"

using namespace sg;

// TODO run tests showing names and check they are correct
// TODO split make tests into rvalue and lvalue
// TODO add function pointer test
// TODO add static_tests for disallowed copy and assignment
// TODO add test moved guard has no effect
// TODO add test to show function can still be called multiple times outside scope guard
// TODO add custom functor tests
// TODO add const functor test
// TODO add member function tests
// TODO add actual exception test
// TODO add actual rollback test
// TODO add temporary test
// TODO add new/delete tests
// TODO add unique_ptr tests
// TODO add shared_ptr tests
// TODO add move into function tests
// TODO add move into container tests
// TODO add tests for descending guard
// TODO add tests for required noexcept
// TODO add tests for no implicitly ignored return (and btw, make sure it would be implicitly ignored)
// TODO for bonus, support function overloads (not sure how or if at all possible)

////////////////////////////////////////////////////////////////////////////////
namespace
{
  auto count = 0u;
  void incc(unsigned& c) { ++c; }
  void inc() { incc(count); }
  void resetc(unsigned& c) { c = 0u; }
  void reset() { resetc(count); }
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A plain function can be used to create a scope_guard")
{
  make_scope_guard(inc);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Demonstration that direct constructor call is possible, but not "
          "advisable")
{
//  scope_guard{inc}; // Error... besides not deducing template args (at least
                      // before C++17), it does not accept everything...

//  scope_guard<decltype(inc)>{inc}; // Error: cannot instantiate data field
                                     //        with function type...

  scope_guard<void(&)()>{
      static_cast<void(&)()>(inc)}; // ... could use ref cast...

  auto& inc_ref = inc;
  scope_guard<decltype(inc_ref)>{inc_ref}; // ... or actual ref...

  scope_guard<decltype(inc)&&>{std::move(inc)}; // ... or even rvalue ref, which
                                                // with functions is treated
                                                // just like an lvalue...


  make_scope_guard(inc); // ... but the BEST is really to use the make function
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A plain-function-based scope_guard executes the function exactly "
          "once when leaving scope")
{
  reset();

  {
    const auto guard = make_scope_guard(inc);
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An lvalue reference to a plain function can be used to create a "
          "scope_guard")
{
  auto& inc_ref = inc;
  make_scope_guard(inc_ref);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An lvalue-reference-to-plain-function-based scope_guard executes "
          "the function exactly once when leaving scope")
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
TEST_CASE("An rvalue reference to a plain function can be used to create a "
          "scope_guard")
{
  make_scope_guard(std::move(inc)); // rvalue ref to function treated as lvalue
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An rvalue-reference-to-plain-function-based scope_guard executes "
          "the function exactly once when leaving scope")
{
  reset();

  {
    const auto guard = make_scope_guard(std::move(inc));
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A reference wrapper to a plain function can be used to create a "
          "scope_guard")
{
  make_scope_guard(std::ref(inc));
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A reference-wrapper-to-plain-function-based scope_guard executes "
          "the function exactly once when leaving scope")
{
  reset();

  {
    const auto guard = make_scope_guard(std::ref(inc));
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A const reference wrapper to a plain function can be used to create "
          "a scope_guard")
{
  make_scope_guard(std::cref(inc));
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A const-reference-wrapper-to-plain-function-based scope_guard "
          "executes the function exactly once when leaving scope")
{
  reset();

  {
    const auto guard = make_scope_guard(std::cref(inc));
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An lvalue std::function that wraps a regular function can be used "
          "to create a scope_guard")
{
  const auto stdf = std::function<decltype(inc)>{inc};
  make_scope_guard(stdf);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard that is created with an "
          "regular-function-wrapping lvalue std::function executes that "
          "std::function exactly once when leaving scope")
{
  count = 0u;

  {
    REQUIRE_FALSE(count);
    const auto stdf = std::function<decltype(inc)>{inc};
    const auto guard = make_scope_guard(stdf);
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An rvalue std::function that wraps a regular function can be used "
          "to create a scope_guard")
{
  make_scope_guard(std::function<decltype(inc)>{inc});
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard that is created with an "
          "regular-function-wrapping rvalue std::function executes that "
          "std::function exactly once when leaving scope")
{
  count = 0u;

  {
    REQUIRE_FALSE(count);
    const auto guard = make_scope_guard(std::function<decltype(inc)>{inc});
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An lvalue reference to a std::function that wraps a regular "
          "function can be used to create a scope_guard")
{
  const auto stdf = std::function<decltype(inc)>{inc};
  const auto& stdf_ref = stdf;
  make_scope_guard(stdf_ref);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard that is created with an "
          "regular-function-wrapping std::function lvalue reference "
          "executes that std::function exactly once when leaving scope")
{
  count = 0u;

  {
    REQUIRE_FALSE(count);
    const auto stdf = std::function<decltype(inc)>{inc};
    const auto& stdf_ref = stdf;
    const auto guard = make_scope_guard(stdf_ref);
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An rvalue reference to a std::function that wraps a regular "
          "function can be used to create a scope_guard")
{
  const auto stdf = std::function<decltype(inc)>{inc};
  make_scope_guard(std::move(stdf));
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard that is created with an "
          "regular-function-wrapping std::function rvalue reference"
          "executes that std::function exactly once when leaving scope")
{
  count = 0u;

  {
    REQUIRE_FALSE(count);
    const auto stdf = std::function<decltype(inc)>{inc};
    const auto guard = make_scope_guard(std::move(stdf));
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
namespace
{
  auto lambda_no_capture_count = 0u;
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A lambda function with no capture can be used to create a "
          "scope_guard")
{
  const auto guard = make_scope_guard([](){});
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A no-capture-lambda-based scope_guard executes the lambda exactly "
          "once when leaving scope")
{
  {
    REQUIRE_FALSE(lambda_no_capture_count);
    const auto guard = make_scope_guard([](){ incc(lambda_no_capture_count); });
    REQUIRE_FALSE(lambda_no_capture_count);
  }

  REQUIRE(lambda_no_capture_count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A lambda function with capture can be used to create a scope_guard")
{
  make_scope_guard([&](){});
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A capturing-lambda-based scope_guard executes the lambda when "
          "leaving scope")
{
  auto lambda_count = 0u;

  {
    const auto guard = make_scope_guard([&lambda_count]()
                                        { incc(lambda_count); });
    REQUIRE_FALSE(lambda_count);
  }

  REQUIRE(lambda_count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard created with a regular-function-wrapping lambda, "
          "calls the lambda exactly once when leaving scope, which in turn "
          "calls the regular function")
{
  count = 0u;
  auto lambda_count = 0u;

  {
    const auto guard = make_scope_guard([&lambda_count]()
                                        { inc(); incc(lambda_count); });
    REQUIRE_FALSE(count);
    REQUIRE_FALSE(lambda_count);
  }

  REQUIRE(count == lambda_count);
  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test lambda function calling std::function")
{
  // TODO
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test std::function calling lambda function")
{
  // TODO
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A bound function can be used to create a scope_guard")
{
  auto boundf_count = 0u;
  make_scope_guard(std::bind(incc, std::ref(boundf_count)));
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A bound-function-based scope_guard calls the bound function exactly "
          "once when leaving scope")
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
TEST_CASE("A bound lambda can be used to create a scope_guard")
{
  make_scope_guard(std::bind([](int /*unused*/){}, 42));
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A bound-lambda-based scope_guard calls the bound lambda exactly "
          "once when leaving scope")
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

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("several levels of indirection involving lambdas, binds, "
          "std::functions, custom functors, and regular functions")
{
  // TODO
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Redundant scope_guards do not interfere with each other - their "
          "combined post-condition holds")
{
  count = 0u;
  auto lambda_count = 0u;

  {
    const auto g1 = make_scope_guard([&lambda_count]()
                                     { inc(); incc(lambda_count); });
    REQUIRE_FALSE(count);
    REQUIRE_FALSE(lambda_count);
    const auto g2 = make_scope_guard([&lambda_count]()
                                     { incc(lambda_count); inc(); });
    REQUIRE_FALSE(count);
    REQUIRE_FALSE(lambda_count);
    const auto g3 = make_scope_guard(inc);
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 3u);
  REQUIRE(lambda_count == 2u);

  const auto g4 = make_scope_guard([&lambda_count]()
                                   { incc(lambda_count); inc(); });
  REQUIRE(count == 3u);
  REQUIRE(lambda_count == 2u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Multiple independent scope_guards do not interfere with each "
          "other - each of their post-conditions holds")
{
  // TODO
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

  // TODO replace with binds
  const auto lvl0_guard = make_scope_guard([&lvl0_count]()
                                           { incc(lvl0_count); });
  REQUIRE_FALSE(lvl0_count);

  {
    const auto lvl1_guard = make_scope_guard([&lvl1_count]()
                                             { incc(lvl1_count); });
    REQUIRE_FALSE(lvl1_count);

    {
      const auto lvl2a_guard = make_scope_guard([&lvl2a_count]()
                                                { incc(lvl2a_count); });
      REQUIRE_FALSE(lvl2a_count);

      {
        const auto lvl3a_guard = make_scope_guard([&lvl3a_count]()
                                                  { incc(lvl3a_count); });
        REQUIRE_FALSE(lvl3a_count);
      }

      REQUIRE(lvl3a_count == 1);
      REQUIRE_FALSE(lvl2a_count);
    }

    REQUIRE(lvl2a_count == 1);
    REQUIRE_FALSE(lvl1_count);
    REQUIRE_FALSE(lvl0_count);

    {
      const auto lvl2b_guard = make_scope_guard([&lvl2b_count]()
                                                { incc(lvl2b_count); });
      REQUIRE_FALSE(lvl2b_count);

      {
        const auto lvl3b_guard = make_scope_guard([&lvl3b_count]()
                                                  { incc(lvl3b_count); });
        REQUIRE_FALSE(lvl3b_count);

        const auto lvl3c_guard = make_scope_guard([&lvl3c_count]()
                                                  { incc(lvl3c_count); });
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
