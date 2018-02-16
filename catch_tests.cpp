#include "scope_guard.hpp"

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch/catch.hpp"

using namespace sg;

// TODO homogenize newlines
// TODO split construction tests into ctor and make
// TODO split make tests into rvalue and lvalue
// TODO replace booleans with counts
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

////////////////////////////////////////////////////////////////////////////////
namespace
{
  bool f_called = false;
  void f()
  {
    f_called = true;
  }
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A plain function can be used to create a scope_guard")
{
  make_scope_guard(f);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A plain-function-based scope_guard executes the function when "
          "leaving scope") // TODO exactly once
{
  f_called = false;

  {
    auto guard = make_scope_guard(f);
    REQUIRE_FALSE(f_called);
  }

  REQUIRE(f_called);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A std::function that wraps a regular function can be used to create "
          "a scope_guard")
{
  make_scope_guard(std::function<decltype(f)>{f});
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("An lvalue std::function that wraps a regular function can be used "
          "to create a scope_guard")
{
  auto stdf = std::function<decltype(f)>{f};
  make_scope_guard(stdf);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard that is created with a regular-function-wrapping "
          "std::function executes the function when leaving scope")
{
  f_called = false;

  {
    REQUIRE_FALSE(f_called);
    auto guard = make_scope_guard(std::function<decltype(f)>{f});
    REQUIRE_FALSE(f_called);
  }

  REQUIRE(f_called);
}

////////////////////////////////////////////////////////////////////////////////
namespace
{
  bool lambda_no_capture_called = false;
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A lambda function with no capture can be used to create a "
          "scope_guard")
{
  auto guard = make_scope_guard([](){});
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A no-capture-lambda-based scope_guard executes the lambda when "
          "leaving scope")
{
  {
    REQUIRE_FALSE(lambda_no_capture_called);
    auto guard = make_scope_guard([](){lambda_no_capture_called = true;});
    REQUIRE_FALSE(lambda_no_capture_called);
  }

  REQUIRE(lambda_no_capture_called);
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
  bool lambda_called = false;

  {
    auto guard = make_scope_guard([&lambda_called](){lambda_called=true;});
    REQUIRE_FALSE(lambda_called);
  }

  REQUIRE(lambda_called);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A scope_guard created with a lambda that calls a regular function "
          "calls the lambda when leaving scope, which in turn calls the "
          "regular function")
{
  f_called = false;
  bool lambda_called = false;

  {
    REQUIRE_FALSE(f_called);
    auto guard = make_scope_guard([&lambda_called](){f(); lambda_called=true;});
    REQUIRE_FALSE(f_called);
    REQUIRE_FALSE(lambda_called);
  }
  REQUIRE(f_called);
  REQUIRE(lambda_called);
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
namespace
{
  void negate_f(bool& b)
  {
    b = !b;
  }
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A bound function can be used to create a scope_guard")
{
  bool boundf_called;
  make_scope_guard(std::bind(negate_f, std::ref(boundf_called)));
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A bound-function-based scope_guard calls the bound function when "
          "leaving scope")
{
  bool boundf_called = false;
  {
    auto guard = make_scope_guard(std::bind(negate_f, std::ref(boundf_called)));
    REQUIRE_FALSE(boundf_called);
  }
  REQUIRE(boundf_called);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A bound lambda can be used to create a scope_guard")
{
  make_scope_guard(std::bind([](int /*unused*/){}, 42));
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("A bound-lambda-based scope_guard calls the bound lambda when "
          "leaving scope")
{
  bool boundl_called = false;
  {
    auto negate_l = [](bool& b){b = !b;};
    auto guard = make_scope_guard(std::bind(negate_l, std::ref(boundl_called)));
    REQUIRE_FALSE(boundl_called);
  }
  REQUIRE(boundl_called);
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
  f_called = false;
  bool lambda_called = false;

  {
    auto g1 = make_scope_guard([&lambda_called](){f(); lambda_called=true;});
    REQUIRE_FALSE(f_called);
    REQUIRE_FALSE(lambda_called);
    auto g2 = make_scope_guard([&lambda_called](){lambda_called=true; f();});
    REQUIRE_FALSE(f_called);
    REQUIRE_FALSE(lambda_called);
    auto g3 = make_scope_guard(f);
    REQUIRE_FALSE(f_called);
  }
  REQUIRE(f_called);
  REQUIRE(lambda_called);

  auto g4 = make_scope_guard([&lambda_called](){lambda_called=true; f();});
  REQUIRE(f_called);
  REQUIRE(lambda_called);
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
  bool lvl0_called = false;
  bool lvl1_called = false;
  bool lvl2a_called = false;
  bool lvl2b_called = false;
  bool lvl3a_called = false;
  bool lvl3b_called = false;
  bool lvl3c_called = false;

  // TODO replace with binds
  auto lvl0_guard = make_scope_guard([&lvl0_called](){lvl0_called = true;});
  REQUIRE_FALSE(lvl0_called);

  {
    auto lvl1_guard = make_scope_guard([&lvl1_called](){lvl1_called = true;});
    REQUIRE_FALSE(lvl1_called);

    {
      auto lvl2a_guard =
          make_scope_guard([&lvl2a_called](){lvl2a_called = true;});
      REQUIRE_FALSE(lvl2a_called);

      {
        auto lvl3a_guard =
          make_scope_guard([&lvl3a_called](){lvl3a_called = true;});
        REQUIRE_FALSE(lvl3a_called);
      }

      REQUIRE(lvl3a_called);
      REQUIRE_FALSE(lvl2a_called);
    }

    REQUIRE(lvl2a_called);
    REQUIRE_FALSE(lvl1_called);
    REQUIRE_FALSE(lvl0_called);

    {
      auto lvl2b_guard =
          make_scope_guard([&lvl2b_called](){lvl2b_called = true;});
      REQUIRE_FALSE(lvl2b_called);

      {
        auto lvl3b_guard =
            make_scope_guard([&lvl3b_called](){lvl3b_called = true;});
        REQUIRE_FALSE(lvl3b_called);

        auto lvl3c_guard =
            make_scope_guard([&lvl3c_called](){lvl3c_called = true;});
        REQUIRE_FALSE(lvl3c_called);
      }

      REQUIRE(lvl3b_called);
      REQUIRE(lvl3c_called);
      REQUIRE_FALSE(lvl2b_called);

    }

    REQUIRE(lvl2b_called);
    REQUIRE_FALSE(lvl1_called);
    REQUIRE_FALSE(lvl0_called);

  }

  REQUIRE(lvl1_called);
  REQUIRE(lvl2a_called);
  REQUIRE(lvl2b_called);
  REQUIRE(lvl3a_called);
  REQUIRE(lvl3b_called);
  REQUIRE(lvl3c_called);
  REQUIRE_FALSE(lvl0_called);

}
