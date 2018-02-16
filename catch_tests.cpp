#include "scope_guard.hpp"

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch/catch.hpp"

using namespace sg;

namespace
{
  //////////////////////////////////////////////////////////////////////////////
  bool f_called = false;

  //////////////////////////////////////////////////////////////////////////////
  void f()
  {
    f_called = true;
  }
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Plain function")
{
  {
    REQUIRE_FALSE(f_called);
    auto guard = make_scope_guard(f);
    REQUIRE_FALSE(f_called);
  }
  REQUIRE(f_called);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Simple std::function")
{
  f_called = false;

  {
    REQUIRE_FALSE(f_called);
    auto guard = make_scope_guard(std::function<decltype(f)>(f));
    REQUIRE_FALSE(f_called);
  }

  REQUIRE(f_called);
}

namespace
{
  //////////////////////////////////////////////////////////////////////////////
  bool lambda_no_capture_called = false;
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test lambda function with no capture")
{
  {
    REQUIRE_FALSE(lambda_no_capture_called);
    auto guard = make_scope_guard([](){lambda_no_capture_called = true;});
    REQUIRE_FALSE(lambda_no_capture_called);
  }

  REQUIRE(lambda_no_capture_called);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test lambda function with capture")
{
  bool lambda_called = false;

  {
    auto guard = make_scope_guard([&lambda_called](){lambda_called=true;});
    REQUIRE_FALSE(lambda_called);
  }
  REQUIRE(lambda_called);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test lambda function calling regular function")
{
  f_called = false;
  bool lambda_called = false;

  {
    REQUIRE_FALSE(f_called);
    REQUIRE_FALSE(lambda_called);
    auto guard = make_scope_guard([&lambda_called](){f(); lambda_called=true;});
    REQUIRE_FALSE(f_called);
    REQUIRE_FALSE(lambda_called);
  }
  REQUIRE(f_called);
  REQUIRE(lambda_called);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test redundant guards")
{
  f_called = false;
  bool lambda_called = false;

  {
    REQUIRE_FALSE(f_called);
    REQUIRE_FALSE(lambda_called);
    auto g1 = make_scope_guard([&lambda_called](){f(); lambda_called=true;});
    REQUIRE_FALSE(f_called);
    REQUIRE_FALSE(lambda_called);
    auto g2 = make_scope_guard([&lambda_called](){lambda_called=true; f();});
    REQUIRE_FALSE(f_called);
    REQUIRE_FALSE(lambda_called);
  }
  REQUIRE(f_called);
  REQUIRE(lambda_called);

  auto g3 = make_scope_guard([&lambda_called](){lambda_called=true; f();});
  REQUIRE(f_called);
  REQUIRE(lambda_called);
}
