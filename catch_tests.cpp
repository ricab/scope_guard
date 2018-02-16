#include "scope_guard.hpp"

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch/catch.hpp"

using namespace sg;

// TODO { cosmetics; }
// TODO split construction tests into ctor and make
// TODO split make tests into rvalue and lvalue
// TODO replace auto with const auto where possible
// TODO add static_tests for disallowed copy and assignment
// TODO add test moved guard has no effect
// TODO add test to show function can still be called multiple times outside scope guard
// TODO add custom functor tests
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
// TODO for bonus, support function overloads (not sure how)

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
TEST_CASE("A plain-function-based scope_guard executes the function exactly "
          "once when leaving scope")
{
  reset();

  {
    auto guard = make_scope_guard(inc);
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 1u);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A std::function that wraps a regular function can be used to create "
          "a scope_guard")
{
  make_scope_guard(std::function<decltype(inc)>{inc});
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An lvalue std::function that wraps a regular function can be used "
          "to create a scope_guard")
{
  auto stdf = std::function<decltype(inc)>{inc};
  make_scope_guard(stdf);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard that is created with a regular-function-wrapping "
          "std::function executes that std::function exactly once when leaving "
          "scope")
{
  count = 0u;

  {
    REQUIRE_FALSE(count);
    auto guard = make_scope_guard(std::function<decltype(inc)>{inc});
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
  auto guard = make_scope_guard([](){});
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A no-capture-lambda-based scope_guard executes the lambda exactly "
          "once when leaving scope")
{
  {
    REQUIRE_FALSE(lambda_no_capture_count);
    auto guard = make_scope_guard([](){incc(lambda_no_capture_count);});
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
    auto guard = make_scope_guard([&lambda_count](){incc(lambda_count);});
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
    auto guard = make_scope_guard([&lambda_count]()
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
    auto guard = make_scope_guard(std::bind(incc, std::ref(boundf_count)));
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
    auto incc_l = [](unsigned& c){ incc(c); };
    auto guard = make_scope_guard(std::bind(incc_l, std::ref(boundl_count)));
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
    auto g1 = make_scope_guard([&lambda_count](){inc(); incc(lambda_count);});
    REQUIRE_FALSE(count);
    REQUIRE_FALSE(lambda_count);
    auto g2 = make_scope_guard([&lambda_count](){incc(lambda_count); inc();});
    REQUIRE_FALSE(count);
    REQUIRE_FALSE(lambda_count);
    auto g3 = make_scope_guard(inc);
    REQUIRE_FALSE(count);
  }

  REQUIRE(count == 3u);
  REQUIRE(lambda_count == 2u);

  auto g4 = make_scope_guard([&lambda_count](){incc(lambda_count); inc();});
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
  auto lvl0_guard = make_scope_guard([&lvl0_count](){incc(lvl0_count);});
  REQUIRE_FALSE(lvl0_count);

  {
    auto lvl1_guard = make_scope_guard([&lvl1_count](){incc(lvl1_count);});
    REQUIRE_FALSE(lvl1_count);

    {
      auto lvl2a_guard =
          make_scope_guard([&lvl2a_count](){incc(lvl2a_count);});
      REQUIRE_FALSE(lvl2a_count);

      {
        auto lvl3a_guard =
          make_scope_guard([&lvl3a_count](){incc(lvl3a_count);});
        REQUIRE_FALSE(lvl3a_count);
      }

      REQUIRE(lvl3a_count == 1);
      REQUIRE_FALSE(lvl2a_count);
    }

    REQUIRE(lvl2a_count == 1);
    REQUIRE_FALSE(lvl1_count);
    REQUIRE_FALSE(lvl0_count);

    {
      auto lvl2b_guard =
          make_scope_guard([&lvl2b_count](){incc(lvl2b_count);});
      REQUIRE_FALSE(lvl2b_count);

      {
        auto lvl3b_guard =
            make_scope_guard([&lvl3b_count](){incc(lvl3b_count);});
        REQUIRE_FALSE(lvl3b_count);

        auto lvl3c_guard =
            make_scope_guard([&lvl3c_count](){incc(lvl3c_count);});
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
